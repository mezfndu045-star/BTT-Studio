// SMO 1.3.0 build only (BTT_V130): game functions used by options that are not ported to 1.3 yet.
// Their 1.3 addresses are unknown, and every imported symbol is resolved when the module loads (a missing one
// aborts), so these local, hidden definitions replace them. The options that call them are disabled in 1.3.
#if BTT_V130

#define BTT_STUB(name, mangled) \
    extern "C" __attribute__((visibility("hidden"), used)) long name() asm(mangled); \
    extern "C" long name() { return 0; }


#endif
