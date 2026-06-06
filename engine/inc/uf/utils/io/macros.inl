#define UF_VFS_MOUNT_CPP( LAMBDA, URI, PRIORITY ) \
namespace {\
    static uf::StaticInitialization TOKEN_PASTE(STATIC_INITIALIZATION_, __LINE__)( []{\
        uf::vfs::mount( LAMBDA( URI, PRIORITY ) );\
    });\
}
