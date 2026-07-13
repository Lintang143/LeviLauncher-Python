#include "mod/MyMod.h"

#include <filesystem>

#include <pl/Mod.hpp>

namespace loader {

ClangeMeMod &ClangeMeMod::instance() {
    static ClangeMeMod instance;
    return instance;
}

ClangeMeMod::ClangeMeMod() : mSelf(*ll::mod::NativeMod::current()) {}

bool ClangeMeMod::load() {
    auto &self = getSelf();
    self.getLogger().debug("Loading...");

    std::error_code ec;
    std::filesystem::create_directories(self.getDataDir(), ec);
    if (ec) {
        self.getLogger().error("Failed to create data directory {}: {}", self.getDataDir().string(),
                               ec.message());
        return false;
    }
    
    std::filesystem::create_directories(self.getConfigDir(), ec);
    if (ec) {
        self.getLogger().error("Failed to create config directory {}: {}",
                               self.getConfigDir().string(), ec.message());
        return false;
    }

    self.getLogger().info("[Logger] Mod Main Folder: {}", self.getModDir().string());
    self.getLogger().info("[Logger] Mod Data Folder: {}", self.getDataDir().string());
    self.getLogger().info("[Logger] Mod Config Folder: {}", self.getConfigDir().string());

    std::filesystem::path modsPath = self.getDataDir() / "mods";
    
    // Cek apakah folder 'mods' sudah ada
    if (std::filesystem::exists(modsPath)) {
        self.getLogger().info("[INFO] 'mods' folder already exist in: {}. cannot create folder", modsPath.string());
    } else {
        self.getLogger().info("'mods' folder not exist. Creating folder in: {}", modsPath.string());
        
        std::filesystem::create_directories(modsPath, ec);
        if (ec) {
            self.getLogger().error("failed while making 'mods' folder:{}", ec.message());
            // return false; // Buka komen ini 
        } else {
            self.getLogger().info("[SUKSES] Folder 'mods' berhasil dibuat untuk pertama kali!");
        }
    }

    mConfigFile.emplace();
    if (!mConfigFile->load()) {
        self.getLogger().warn("Failed to load typed config");
        return false;
    }
    mConfig = mConfigFile->value();

    self.getLogger().info("Loaded {} from {}", self.getName(), self.getModDir().string());
    return true;
}

bool ClangeMeMod::enable() {
    auto &self = getSelf();
    self.getLogger().debug("Enabling...");
    if (!mConfig.enabled) {
        self.getLogger().info("clange_me is disabled by config");
        return true;
    }

    self.getLogger().info("Config message: {}", mConfig.message);
    return true;
}

bool ClangeMeMod::disable() {
    getSelf().getLogger().debug("Disabling...");
    // Undo enable-time state here.
    return true;
}

bool ClangeMeMod::unload() {
    getSelf().getLogger().debug("Unloading...");
    // Release load-time resources here.
    mConfigFile.reset();
    return true;
}

} // namespace clange_me
