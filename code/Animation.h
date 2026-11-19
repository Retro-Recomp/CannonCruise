// C++98 compatible headers and padding used for byte-identical reproduction
#pragma pack(push, 1)

class Animation {
public:
    // Static factory callback signature (retn 18h = 6 DWORD args)
    static void* Load(char const *pStream);
    
    // Static unloader callback signature (retn 8 = 2 DWORD args)
    static int Unload(void *pResource);

private:
    struct ResourceData {
        unsigned char pad_00[16]; 
        int elementCount; // Must match assembly logic
    };
};

#pragma pack(pop)