#include "mod/MyMod.h"
#include <filesystem>
#include <pl/Mod.hpp>          //[span_0](start_span)[span_0](end_span)
#include "pl/LuaBinding.hpp"  // Include our native binding file

namespace clange_me {

ClangeMeMod &ClangeMeMod::instance() {
    static ClangeMeMod instance;
    return instance;
}

ClangeMeMod::ClangeMeMod() : mSelf(*ll::mod::NativeMod::current()), L(nullptr) {} //[span_1](start_span)[span_1](end_span)

bool ClangeMeMod::load() {
    auto &self = getSelf(); //[span_2](start_span)[span_2](end_span)
    self.getLogger().debug("Loading..."); //[span_3](start_span)[span_3](end_span)

    std::error_code ec;
    std::filesystem::create_directories(self.getDataDir(), ec); //[span_4](start_span)[span_4](end_span)
    if (ec) {
        self.getLogger().error("Failed to create data directory {}: {}", self.getDataDir().string(), ec.message()); //[span_5](start_span)[span_5](end_span)
        return false;
    }
    
    std::filesystem::create_directories(self.getConfigDir(), ec); //[span_6](start_span)[span_6](end_span)
    if (ec) {
        self.getLogger().error("Failed to create config directory {}: {}", self.getConfigDir().string(), ec.message()); //[span_7](start_span)[span_7](end_span)
        return false;
    }

    self.getLogger().info("[Logger] Mod Main Folder: {}", self.getModDir().string()); //[span_8](start_span)[span_8](end_span)
    self.getLogger().info("[Logger] Mod Data Folder: {}", self.getDataDir().string()); //[span_9](start_span)[span_9](end_span)
    self.getLogger().info("[Logger] Mod Config Folder: {}", self.getConfigDir().string()); //[span_10](start_span)[span_10](end_span)

    std::filesystem::path modsPath = self.getDataDir() / "mods"; //[span_11](start_span)[span_11](end_span)
    
    // Check if the 'mods' folder already exists
    if (std::filesystem::exists(modsPath)) { //[span_12](start_span)[span_12](end_span)
        self.getLogger().info("[INFO] 'mods' folder already exists in: {}. Cannot create folder", modsPath.string()); //[span_13](start_span)[span_13](end_span)
    } else {
        self.getLogger().info("'mods' folder does not exist. Creating folder in: {}", modsPath.string()); //[span_14](start_span)[span_14](end_span)
        std::filesystem::create_directories(modsPath, ec); //[span_15](start_span)[span_15](end_span)
        if (ec) {
            self.getLogger().error("Failed to create 'mods' folder: {}", ec.message()); //[span_16](start_span)[span_16](end_span)
        } else {
            self.getLogger().info("[SUCCESS] 'mods' folder successfully created for the first time!");
        }
    }

    mConfigFile.emplace(); //[span_17](start_span)[span_17](end_span)
    if (!mConfigFile->load()) { //[span_18](start_span)[span_18](end_span)
        self.getLogger().warn("Failed to load typed config"); //[span_19](start_span)[span_19](end_span)
        return false;
    }
    mConfig = mConfigFile->value(); //[span_20](start_span)[span_20](end_span)

    self.getLogger().info("Loaded {} from {}", self.getName(), self.getModDir().string()); //[span_21](start_span)[span_21](end_span)
    return true;
}

bool ClangeMeMod::enable() {
    auto &self = getSelf(); //[span_22](start_span)[span_22](end_span)
    self.getLogger().debug("Enabling..."); //[span_23](start_span)[span_23](end_span)
    
    if (!mConfig.enabled) { //[span_24](start_span)[span_24](end_span)
        self.getLogger().info("clange_me is disabled by config"); //[span_25](start_span)[span_25](end_span)
        return true;
    }

    self.getLogger().info("Config message: {}", mConfig.message); //[span_26](start_span)[span_26](end_span)

    // --- INITIALIZE & RUN LUA SUBSYSTEM ---
    self.getLogger().info("[Lua Engine] Initializing Lua State...");
    L = luaL_newstate();
    if (!L) {
        self.getLogger().error("[Lua Engine] Failed to create Lua State!");
        return false;
    }

    luaL_openlibs(L);
    pl::lua_binding::register_all_pl_modules(L);

    std::filesystem::path modsPath = self.getDataDir() / "mods"; //[span_27](start_span)[span_27](end_span)

    if (std::filesystem::exists(modsPath)) {
        self.getLogger().info("[Lua Engine] Scanning Lua script files in: {}", modsPath.string());
        
        for (const auto& entry : std::filesystem::directory_iterator(modsPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".lua") {
                self.getLogger().info("[Lua Engine] Executing script: {}", entry.path().filename().string());
                
                if (luaL_dofile(L, entry.path().string().c_str()) != LUA_OK) {
                    self.getLogger().error("[Lua Error] Failed to load {}: {}", 
                                           entry.path().filename().string(), lua_tostring(L, -1));
                    lua_pop(L, 1);
                }
            }
        }
    }

    // Trigger the 'enable' lifecycle event registered inside the Lua scripts
    self.getLogger().info("[Lua Engine] Triggering script enable event...");
    pl::lua_binding::triggerLuaLifecycle(L, pl::lua_binding::luaEnableRef);

    return true;
}

bool ClangeMeMod::disable() {
    auto &self = getSelf(); //[span_28](start_span)[span_28](end_span)
    self.getLogger().debug("Disabling..."); //[span_29](start_span)[span_29](end_span)

    if (L) {
        self.getLogger().info("[Lua Engine] Triggering script disable event...");
        // Trigger the 'disable' lifecycle event inside the Lua scripts
        pl::lua_binding::triggerLuaLifecycle(L, pl::lua_binding::luaDisableRef);
        
        self.getLogger().info("[Lua Engine] Closing Lua State...");
        lua_close(L);
        L = nullptr;
    }
    return true;
}

bool ClangeMeMod::unload() {
    auto &self = getSelf(); //[span_30](start_span)[span_30](end_span)
    self.getLogger().debug("Unloading..."); //[span_31](start_span)[span_31](end_span)
    
    // Simple log indicating Lua has completely stopped
    self.getLogger().info("[Lua Engine] Lua subsystem has been completely unloaded.");

    mConfigFile.reset(); //[span_32](start_span)[span_32](end_span)
    return true;
}

} // namespace clange_me
