#include "BPKG.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <assert.h>

using std::ios;
using std::ifstream;
using std::ofstream;

namespace Brisk
{
	constexpr size_t index_t_size = sizeof(BriskPKG::index_t);
	constexpr size_t version_t_size = sizeof(BriskPKG::version_t);

	BriskPKG::BriskPKG(const std::string& bpkg, bool signatureCheck) : bpkgPath(bpkg)
	{
		// Get file
		ifstream bpkgFile(bpkg, ios::binary);
		if(!bpkgFile) throw std::runtime_error("Could not find BPK file \"" + bpkg + "\".");
		
		// Get version
		uint8_t versionBytes[version_t_size];
		getBytes(versionBytes, &bpkgFile, version_t_size);
		version = convert<version_t>(versionBytes);

		// Get size
		bpkgSize = fileSize(&bpkgFile);
		
		std::string currentPath;
		while(bpkgFile.tellg() < bpkgSize)
		{
			// Check message
			char* msgCheck = new char[messageSize];
			bpkgFile.read(msgCheck, messageSize);
			
			bool foundPathEnd = true;
			bool foundHeaderEnd = true;
			for(size_t i = 0; i < messageSize; i++)
			{
				foundPathEnd &= msgCheck[i] == pathEnd[i];
				foundHeaderEnd &= msgCheck[i] == headerEnd[i];
			}
			delete[] msgCheck;
			
			if(foundHeaderEnd)
			{
				binStart = (index_t) bpkgFile.tellg();
				break;
			}
			else bpkgFile.seekg((index_t) bpkgFile.tellg() - messageSize);
			
			if(foundPathEnd)
			{
				// Get offset position
				const index_t offsetPos = (index_t) bpkgFile.tellg() + 1;
				
				// Define byte arrays
				uint8_t offsetBytes[index_t_size];
				uint8_t sizeBytes[index_t_size];

				// Read
				bpkgFile.seekg(offsetPos);
				getBytes(offsetBytes, &bpkgFile, index_t_size);
				getBytes(sizeBytes, &bpkgFile, index_t_size);
				
				// Convert to index_t
				index_t offset = convert<index_t>(offsetBytes);
				index_t size = convert<index_t>(sizeBytes);
				
				// Generate header entry
				header[currentPath] = HeaderEntry { offset, size };
				currentPath.clear();
			}
			else
			{
				// Add current char to local path
				char* currentChar = new char;
				bpkgFile.read(currentChar, 1);
				currentPath += currentChar;
				delete currentChar;
			}
		}
	}

	void BriskPKG::getBytes(uint8_t* out, ifstream* file, size_t width)
	{
		char* dataChars = new char[width];
		file->read(dataChars, width);
		for(size_t i = 0; i < width; i++) out[i] = (uint8_t) dataChars[i];
		delete[] dataChars;
	}

	const BriskPKG::ObjectEntry* BriskPKG::load(const std::string& path)
	{
		if(objects.find(path) == objects.end())
		{
			// Object not loaded, load it in
			if(header.find(path) == header.end()) return nullptr;
			HeaderEntry entry = header.find(path)->second;
			
			// Read data into the object entry
			ifstream bpkg(bpkgPath, ios::binary);
			if(!bpkg) return nullptr;
			
			bpkg.seekg(binStart + entry.offset);
			char* data = new char[entry.size];	// Destroyed in ~ObjectEntry()
			bpkg.read(data, entry.size);
			
			// Create object
			objects[path] = ObjectEntry { reinterpret_cast<uint8_t*>(data), entry.size };
			return &objects.find(path)->second;
		}
		else return &objects.find(path)->second;
	}
	
	BriskPKG::ObjectEntry::~ObjectEntry() { delete[] data; }
	
	void BriskPKG::unload(const std::string& path) { objects.erase(path); }
	void BriskPKG::clear() { objects.clear(); }
	
	void BriskPKG::generate
	(
		const std::string& sources,
		const std::string& output,
		version_t version,
		unsigned int bufferSize
	)
	{
		namespace fs = std::filesystem;
		assert(bufferSize > 0);
		
		// Temporary Paths
		const std::string binFile = output + ".bin";
		const std::string headerFile = output + ".header";
		
		// Entry vector
		struct HeaderEntry { std::string path; index_t offset; index_t size; };
		std::vector<HeaderEntry> entries;
		
		// Generate binary
		index_t totalIndex = 0;
		ofstream binOut(binFile, ios::binary);
		for(const fs::directory_entry& file : fs::directory_iterator(sources))
		{
			// Load file
			const std::string path = file.path().string();
			if(path == binFile) continue;
			
			ifstream in(path, ios::binary);
			
			// Check if file is readable
			if(!in)
			{
				std::string message = "Failed to open file \"";
				message += path;
				message += "\" for BPK \"";
				message += output;
				message += "\".";
				std::cerr << message << std::endl;
				continue;
			}
			
			// Get size
			index_t size = fileSize(&in);
			
			// Loop through in manageable chunks
			index_t index = 0;
			while(index < size)
			{
				// Set offset
				in.seekg(index, ios::beg);
				
				// Calculate buffer size
				index_t buffer = getBufferSize(bufferSize, size, index);
				
				// Read
				copyFile(&in, &binOut, buffer);
				index += buffer;
			}
			in.close();
			
			entries.push_back(HeaderEntry { path, totalIndex, index });
			totalIndex += index;
		}
		
		// Get total bin file size
		index_t binSize = fileSize(&binOut);
		binOut.close();
		
		// Generate header
		index_t i = 0;
		ofstream headerOut(headerFile, ios::binary);
		for(const HeaderEntry& entry : entries)
		{
			// Write path
			headerOut.write(entry.path.c_str(), entry.path.length());
			headerOut.write(pathEnd, messageSize);
			
			// Convert index_t to uint8_t[8]
			uint8_t writeableOffset[index_t_size];
			uint8_t writeableSize[index_t_size];
			convert(entry.offset + (i * messageSize), writeableOffset);
			convert(entry.size + (i * messageSize), writeableSize);
			
			// Write offset
			headerOut.write(reinterpret_cast<const char*>(writeableOffset), index_t_size);
			headerOut.write(reinterpret_cast<const char*>(writeableSize), index_t_size);
			
			i++;
		}
		headerOut.write(headerEnd, messageSize);
		index_t headerSize = fileSize(&headerOut);
		headerOut.close();
		
		// Generate final file
		ofstream out(output, ios::binary);
		index_t index = 0;
		
		// Write Header
		ifstream headerIn(headerFile, ios::binary);
		while(index < headerSize)
		{
			// Get buffer size
			index_t buffer = getBufferSize(bufferSize, headerSize, index);
			
			// Write
			copyFile(&headerIn, &out, buffer);
			index += buffer;
		}
		headerIn.close();
		
		// Write binary
		ifstream binIn(binFile, ios::binary);
		index_t size = headerSize + binSize;
		while(index < size)
		{
			// Get buffer size
			index_t buffer = getBufferSize(bufferSize, size, index);
			
			// Write
			copyFile(&binIn, &out, buffer);
			index += buffer;
		}
		binIn.close();
		out.close();
		
		// Delete temporary files
		fs::remove(binFile);
		fs::remove(headerFile);
	}
	
	constexpr BriskPKG::index_t BriskPKG::getBufferSize(unsigned int max, index_t size, index_t index)
	{
		index_t leftover = size - index;
		
		if(leftover < max) return leftover;
		else return max;
	}
	
	template<typename T> void BriskPKG::convert(T in, uint8_t out[])
	{
		for(size_t i = 0; i < sizeof(T); i++)
		{
			out[i] = uint8_t((in >> 8 * ((sizeof(T) - 1) - i)) & 0xFFu);
		}
	}
	
	template<typename T> T BriskPKG::convert(const uint8_t in[])
	{
		T val = 0u;
		for(size_t i = 0; i < sizeof(T); i++)
		{
			size_t shift = sizeof(T);
			shift *= sizeof(T) - 1u - i;

			val |= T(in[i]) << T(shift);
		}
		return val;
	}
	
	void BriskPKG::copyFile(ifstream* in, ofstream* out, unsigned int bufferSize)
	{
		char* data = new char[bufferSize];
		in->read(data, bufferSize);
		out->write(data, bufferSize);
		delete[] data;
	}
	
	BriskPKG::index_t BriskPKG::fileSize(std::ifstream* file)
	{
		index_t oldPosition = file->tellg();
		file->seekg(0, ios::end);
		index_t size = file->tellg();
		file->seekg(oldPosition);
		return size;
	}
	
	BriskPKG::index_t BriskPKG::fileSize(std::ofstream* file)
	{
		index_t oldPosition = file->tellp();
		file->seekp(0, ios::end);
		index_t size = file->tellp();
		file->seekp(oldPosition);
		return size;
	}
}
