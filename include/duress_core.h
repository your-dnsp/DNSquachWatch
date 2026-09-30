#pragma once
#include <stdint.h>
#include <stddef.h>
namespace Duress {
enum class Boot : uint8_t { NORMAL, PENDING, DECOY, FAULT };
enum Error : uint32_t { NVS=1, BLACKBOX=2, COREDUMP=4, SD_IO=8, SD_LIMIT=16, JOURNAL=32 };
struct Node { char name[96]; bool directory; };
// All writes below are checked by the platform adapter, including readback.
struct Storage {
    virtual ~Storage() = default;
    virtual bool readJournal(uint32_t offset, void*, size_t)=0;
    virtual bool eraseJournal()=0;
    virtual bool writeJournal(uint32_t offset, const void*, size_t)=0;
    virtual bool eraseBlock(unsigned region, unsigned sector)=0;
    virtual bool mountCard()=0;
    // 1 entry, 0 end, -1 I/O error. No open handles survive this call.
    virtual int list(const char* path, unsigned index, Node&)=0;
    virtual bool eraseFile(const char* path)=0;
    virtual bool eraseDirectory(const char* path)=0;
    virtual bool unmountCard()=0;
};
Boot inspect(Storage&, uint32_t* errors=nullptr);
bool arm(Storage&); // Durable intent BEFORE deleting anything; failure leaves lock intact.
bool appName(const char*);
bool safeLeaf(const char*);
class Wipe {
public:
    void begin(uint32_t now);
    void tick(Storage&, uint32_t now);
    bool done() const { return finished; }
    uint32_t errors() const { return failures; }
private:
    struct Frame { char path[256]; unsigned index; } stack[9]{};
    unsigned region=0, sector=0, depth=0, visited=0;
    uint32_t started=0, failures=0;
    bool intentChecked=false, mounted=false, cardStarted=false, finished=false;
    void finish(Storage&);
};
}
