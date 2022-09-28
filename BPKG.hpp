#ifndef BRISKPKG_BPKG_HPP
#define BRISKPKG_BPKG_HPP

#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace Brisk
{
	class BriskPKG
	{
	public:
		typedef uint64_t index_t;
		typedef uint32_t version_t;	// Reserve 4 bytes of space to store a version

		// Signature to check whether this is auto-set by the BPK.
		static constexpr version_t revisionSignature = 'B' << 24 | 'P' << 16 | 'K' << 8;
		static constexpr version_t revision = 0 | revisionSignature;

		// BPKG Version
		version_t version;

		// Constructor
		BriskPKG(const std::string& bpkg, bool signatureCheck = true);
		
		// Entry management
		struct ObjectEntry
		{
			uint8_t* data;
			index_t size;

			~ObjectEntry();
		};
		const ObjectEntry* load(const std::string& path);
		void unload(const std::string& path);
		void clear();

		static void generate
		(
			const std::string& sources,
			const std::string& output,
			version_t version = revision,
			unsigned int bufferSize = defaultBufferSize
		);
	protected:
		struct HeaderEntry { index_t offset; index_t size; };
		std::unordered_map<std::string, HeaderEntry> header;
		std::unordered_map<std::string, ObjectEntry> objects;
		
		const std::string bpkgPath;
		index_t bpkgSize;
		index_t binStart;
		
		// Messages
		static constexpr size_t messageSize = 1;
		static constexpr char pathEnd[] = { 0x19 };
		static constexpr char headerEnd[] = { 0x1D };
	private:
		static constexpr unsigned int defaultBufferSize = 32 * 1024; // 32KB
		static constexpr index_t getBufferSize(unsigned int max, index_t size, index_t index);
		
		static void getBytes(uint8_t* out, std::ifstream* file, size_t width);

		template<typename T> static void convert(T in, uint8_t out[]);
		template<typename T> static T convert(const uint8_t in[]);
		
		static void copyFile(std::ifstream* in, std::ofstream* out, unsigned int bufferSize);
		
		static index_t fileSize(std::ifstream* file);
		static index_t fileSize(std::ofstream* file);
	};
}
#endif