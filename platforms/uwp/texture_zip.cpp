#include "texture_zip.hpp"
#include <zlib.h>
#include <array>
#include <algorithm>
#include "texture_storage.hpp"
#include <SDL3/SDL.h>
#include <vector>
#include <unordered_set>
#include <stdexcept>
#include <cctype>
namespace {
uint16_t u16(const uint8_t* b) { return b[0] | (uint16_t(b[1]) << 8); }
uint32_t u32(const uint8_t* b) { return u16(b) | (uint32_t(u16(b + 2)) << 16); }
[[noreturn]] void fail(const char* text) { throw std::runtime_error(text); }
struct TextureIO {
  SDL_IOStream* io = nullptr;
  bool good = false;
  explicit TextureIO(const std::filesystem::path& path, const char* mode) {
    const auto name = path.u8string();
    io = SDL_IOFromFile(reinterpret_cast<const char*>(name.c_str()), mode); good = io != nullptr;
  }
  ~TextureIO() { if (io) SDL_CloseIO(io); }
  TextureIO(const TextureIO&) = delete;
  explicit operator bool() const { return good; }
  bool read(void* data, size_t count) { return io && SDL_ReadIO(io, data, count) == count; }
  void write(const void* data, size_t count) { good = io && SDL_WriteIO(io, data, count) == count; }
  void close() { if (io) { good = SDL_CloseIO(io) && good; io = nullptr; } }
};
void read(TextureIO& file, uint64_t offset, void* data, size_t count) {
  if (SDL_SeekIO(file.io, static_cast<Sint64>(offset), SDL_IO_SEEK_SET) < 0 || !file.read(data, count))
    fail("ZIP incompleto o ilegible.");
}
std::string lower(std::string value) {
  for (char& c : value) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
  return value;
}
bool validPath(std::string path) {
  std::replace(path.begin(), path.end(), static_cast<char>(92), static_cast<char>(47));
  if (path.empty() || path.front() == '/' || path.find(':') != std::string::npos) return false;
  size_t start = 0;
  while (start < path.size()) {
    size_t end = path.find('/', start); if (end == std::string::npos) end = path.size();
    const auto part = path.substr(start, end - start);
    if (part == ".." || part == ".") return false;
    for (unsigned char c : part) if (c < 32) return false;
    start = end + 1;
  }
  return true;
}
bool gameRoot(std::string path) {
  path = "/" + lower(path);
  return path.find("/gsa/") != std::string::npos || path.find("/gsae01/") != std::string::npos ||
      path.find("/gsap01/") != std::string::npos || path.find("/gsaj01/") != std::string::npos;
}
struct Entry { std::string path, name; uint32_t crc, compressed, size, offset; uint16_t method; };
bool textureValid(const std::filesystem::path& path, uint64_t size) {
  TextureIO file(path, "rb");
  std::array<uint8_t, 148> h{}; file.read(reinterpret_cast<char*>(h.data()), h.size());
  const auto name = lower(path.filename().string());
  if (name.ends_with(".png")) {
    const uint8_t signature[] = {137,80,78,71,13,10,26,10};
    if (size < 33 || !std::equal(signature, signature + 8, h.begin())) return false;
    auto big = [&](size_t i) { return (uint32_t(h[i]) << 24) | (uint32_t(h[i+1]) << 16) | (uint32_t(h[i+2]) << 8) | h[i+3]; };
    return u32(h.data()+12) == 0x52444849 && big(16) > 0 && big(20) > 0 &&
        big(16) <= 8192 && big(20) <= 8192;
  }
  if (size < 128 || u32(h.data()) != 0x20534444 || u32(h.data()+4) != 124 || u32(h.data()+76) != 32) return false;
  const uint32_t width = u32(h.data()+16), height = u32(h.data()+12);
  if (!width || !height || width > 8192 || height > 8192 || (u32(h.data()+112) & 0x20FE00)) return false;
  const auto code = u32(h.data()+84);
  uint32_t block = 0, bytesPerPixel = 0, offset = 128;
  if (code == 0x31545844) block = 8;
  else if (code == 0x35545844) block = 16;
  else if (code == 0x30315844) {
    offset = 148;
    if (size < offset || u32(h.data()+132) != 3 || u32(h.data()+140) != 1 || (u32(h.data()+136)&4)) return false;
    const auto format = u32(h.data()+128);
    if (format == 71) block = 8;
    else if (format == 77 || format == 83 || format == 98) block = 16;
    else if (format == 28 || format == 87) bytesPerPixel = 4;
    else return false;
  } else return false;
  if (block && (width % 4 || height % 4)) return false;
  const uint32_t mips = std::max(1u, u32(h.data()+28));
  uint32_t maxMips = 1;
  for (auto v = std::max(width, height); v > 1; v >>= 1) ++maxMips;
  if (mips > maxMips) return false;
  if (mips > 1 && (!(u32(h.data()+8)&0x20000) || !(u32(h.data()+108)&8) || !(u32(h.data()+108)&0x400000))) return false;
  uint64_t expected = offset;
  for (uint32_t i = 0; i < mips; ++i) {
    const uint64_t w = std::max(1u, width >> i), hgt = std::max(1u, height >> i);
    expected += block ? ((w+3)/4)*((hgt+3)/4)*block : w*hgt*bytesPerPixel;
  }
  return size >= expected;
}
}
FhTexturePackResult fhExtractTextureZip(const std::filesystem::path& zip,
    const std::filesystem::path& output, FhTextureProgress progress, FhTextureCancelled cancelled) {
  TextureIO file(zip, "rb");
  if (!file) fail("No se pudo abrir el ZIP.");
  const auto end = SDL_GetIOSize(file.io);
  if (end < 22) fail("El archivo no es un ZIP compatible.");
  const uint64_t size = static_cast<uint64_t>(end), tailSize = std::min<uint64_t>(size, 65557);
  std::vector<uint8_t> tail(static_cast<size_t>(tailSize)); read(file,size-tailSize,tail.data(),tail.size());
  size_t eocd = tail.size();
  for (size_t i = tail.size()-22;; --i) {
    if (u32(tail.data()+i)==0x06054b50 && i+22+u16(tail.data()+i+20)==tail.size()) { eocd=i; break; }
    if (!i) break;
  }
  if (eocd==tail.size()) fail("ZIP sin directorio valido.");
  const auto* e=tail.data()+eocd;
  const uint32_t count=u16(e+10), cdSize=u32(e+12), cdOffset=u32(e+16);
  if (u16(e+4)||u16(e+6)||count!=u16(e+8)||count==65535||cdOffset==0xffffffff||cdSize==0xffffffff)
    fail("ZIP dividido o ZIP64 no compatible. Usa un ZIP normal.");
  if (!count||count>20000||cdSize>16*1024*1024||uint64_t(cdOffset)+cdSize>size-tailSize+eocd)
    fail("Directorio ZIP invalido o demasiado grande.");
  std::vector<Entry> entries;
  uint32_t ignoredNames = 0;
  uint64_t cursor=cdOffset;
  for (uint32_t i=0;i<count;++i) {
    if (cancelled && cancelled()) fail("Importacion cancelada.");
    std::array<uint8_t,46> h{}; read(file,cursor,h.data(),h.size());
    if (u32(h.data())!=0x02014b50) fail("Directorio ZIP danado.");
    const uint16_t nameLength=u16(h.data()+28), extraLength=u16(h.data()+30), commentLength=u16(h.data()+32);
    if (!nameLength||nameLength>4096||cursor+46+nameLength+extraLength+commentLength>uint64_t(cdOffset)+cdSize)
      fail("Nombre o entrada ZIP invalida.");
    std::string path(nameLength, static_cast<char>(0)); read(file,cursor+46,path.data(),path.size());
  std::replace(path.begin(), path.end(), static_cast<char>(92), static_cast<char>(47));
    if (!validPath(path)) fail("El ZIP contiene una ruta no permitida.");
    if (u16(h.data()+8)&1) fail("No se admiten ZIP con contrasena.");
    cursor+=46+nameLength+extraLength+commentLength;
    if (path.back()=='/') continue;
    const auto slash=path.find_last_of('/'); const auto name=path.substr(slash==std::string::npos?0:slash+1);
    const auto key=lower(name);
    if (!key.starts_with("tex1_")||(!key.ends_with(".dds")&&!key.ends_with(".png"))) continue;
    if (key.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_.$-")!=std::string::npos) { ++ignoredNames; continue; }
    const auto compressed=u32(h.data()+20), decoded=u32(h.data()+24);
    const uint16_t method=u16(h.data()+10);
    if ((method!=0&&method!=8)||decoded>128*1024*1024||compressed>128*1024*1024||u16(h.data()+34))
      fail("Compresion o tamano de textura no compatible.");
    entries.push_back({path,name,u32(h.data()+16),compressed,decoded,u32(h.data()+42),method});
  }
  if (entries.empty()) fail("El ZIP no contiene texturas DDS o PNG con nombres tex1_.");
  const bool preferGame=std::any_of(entries.begin(),entries.end(),[](const auto& x){return gameRoot(x.path);});
  std::sort(entries.begin(),entries.end(),[](const auto& a,const auto& b){return lower(a.path)<lower(b.path);});
  uint64_t total=0;
  for(const auto& x:entries) total+=x.size;
  if(total>8ull*1024*1024*1024) fail("El pack supera 8 GB sin comprimir.");
  fhTextureStorage::create(output);
  if (!fhTextureStorage::empty(output)) fail("La carpeta temporal no esta vacia.");
  FhTexturePackResult result;
  result.skipped = ignoredNames;
  uint64_t done=0;
  std::unordered_set<std::string> names;
  std::array<uint8_t,64*1024> input{}, decoded{};
  for(const auto& x:entries) {
    if(cancelled&&cancelled()) fail("Importacion cancelada.");
    const auto key=lower(x.name);
    if ((preferGame&&!gameRoot(x.path))||names.contains(key)) {
      ++result.skipped; done+=x.size; if(progress)progress(done,total); continue;
    }
    std::array<uint8_t,30> local{}; read(file,x.offset,local.data(),local.size());
    if(u32(local.data())!=0x04034b50||u16(local.data()+8)!=x.method||(u16(local.data()+6)&1))
      fail("Cabecera ZIP de textura invalida.");
    const uint64_t offset=uint64_t(x.offset)+30+u16(local.data()+26)+u16(local.data()+28);
    if(offset+x.compressed>cdOffset) fail("Textura ZIP fuera de rango.");
    const auto target=output/x.name;
    TextureIO dest(target,"wb");
    if(!dest) fail("No hay espacio o no se pudo crear la textura.");
    uint64_t written=0;
    uLong crc=crc32(0,Z_NULL,0);
    auto write=[&](const uint8_t* data,size_t bytes) {
      if(written+bytes>x.size)fail("Tamano de textura ZIP incorrecto.");
      dest.write(reinterpret_cast<const char*>(data),bytes);
      if(!dest)fail("No se pudo escribir la textura. Comprueba espacio libre.");
      crc=crc32(crc,data,static_cast<uInt>(bytes)); written+=bytes;
      if(progress)progress(done+written,total);
      if(cancelled&&cancelled())fail("Importacion cancelada.");
    };
    if(x.method==0) {
      if(x.compressed!=x.size)fail("Entrada ZIP sin comprimir invalida.");
      for(uint64_t pos=0;pos<x.compressed;) {
        const size_t bytes=static_cast<size_t>(std::min<uint64_t>(input.size(),x.compressed-pos));
        read(file,offset+pos,input.data(),bytes); write(input.data(),bytes); pos+=bytes;
      }
    } else {
      z_stream stream{};
      if(inflateInit2(&stream,-MAX_WBITS)!=Z_OK)fail("No se pudo iniciar la descompresion.");
      struct End { z_stream* p; ~End(){inflateEnd(p);} } cleanup{&stream};
      uint64_t loaded=0; int status=Z_OK;
      while(status!=Z_STREAM_END) {
        if(cancelled&&cancelled())fail("Importacion cancelada.");
        if(!stream.avail_in && loaded<x.compressed) {
          const size_t bytes=static_cast<size_t>(std::min<uint64_t>(input.size(),x.compressed-loaded));
          read(file,offset+loaded,input.data(),bytes); loaded+=bytes; stream.next_in=input.data(); stream.avail_in=static_cast<uInt>(bytes);
        }
        stream.next_out=decoded.data(); stream.avail_out=static_cast<uInt>(decoded.size());
        status=inflate(&stream,Z_NO_FLUSH);
        if(status!=Z_OK&&status!=Z_STREAM_END)fail("Textura ZIP comprimida danada.");
        const size_t bytes=decoded.size()-stream.avail_out;
        write(decoded.data(),bytes);
        if(status!=Z_STREAM_END&&!bytes&&!stream.avail_in&&loaded==x.compressed)fail("Textura ZIP incompleta.");
      }
      if(stream.total_in!=x.compressed)fail("Datos ZIP adicionales inesperados.");
    }
    dest.close();
    if(!dest||written!=x.size||static_cast<uint32_t>(crc)!=x.crc)fail("La comprobacion CRC del ZIP fallo.");
    if(!textureValid(target,written)) {
      fhTextureStorage::remove(target); ++result.skipped;
      if (result.warnings.size() < 32) result.warnings.push_back("Skipped incompatible texture: " + x.name);
    }
    else {names.insert(key); ++result.installed; result.bytes+=written;}
    done+=x.size; if(progress)progress(done,total);
  }
  if(!result.installed)fail("Ninguna textura del ZIP es compatible.");
  return result;
}
