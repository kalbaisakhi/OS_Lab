#include <iostream>
#include <string>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <fstream>
#include <map>
#include <vector>
#include <cstring>

// Global variables
std::map<uid_t, std::string> uidCache;  // Cache for UID to login ID mapping
int fileCount = 0;                      // Counter for matching files

// Function to convert UID to login ID
std::string getLoginFromUID(uid_t uid) {
    // Check if already in cache
    if (uidCache.find(uid) != uidCache.end()) {
        return uidCache[uid];
    }
    
    // Not in cache, look up in /etc/passwd
    struct passwd *pwd = getpwuid(uid);
    if (pwd != nullptr) {
        uidCache[uid] = std::string(pwd->pw_name);
        return uidCache[uid];
    }
    
    // If not found, return UID as string
    return std::to_string(uid);
}

// Function to check if a filename has the given extension
bool hasExtension(const std::string& filename, const std::string& ext) {
    if (filename.length() <= ext.length() + 1) {
        return false;
    }
    
    size_t pos = filename.rfind("." + ext);
    return (pos != std::string::npos && pos == filename.length() - ext.length() - 1);
}

// Recursive directory traversal function
void findFiles(const std::string& dirPath, const std::string& ext) {
    DIR* dir = opendir(dirPath.c_str());
    if (dir == nullptr) {
        std::cerr << "Error: Cannot open directory " << dirPath << std::endl;
        return;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        
        // Skip . and .. directories
        if (name == "." || name == "..") {
            continue;
        }
        
        std::string fullPath = dirPath;
        if (fullPath.back() != '/') {
            fullPath += '/';
        }
        fullPath += name;
        
        struct stat statbuf;
        if (lstat(fullPath.c_str(), &statbuf) == -1) {
            std::cerr << "Error: Cannot get status of " << fullPath << std::endl;
            continue;
        }
        
        if (S_ISDIR(statbuf.st_mode)) {
            // Recursively search subdirectories
            findFiles(fullPath, ext);
        } else if (S_ISREG(statbuf.st_mode) && hasExtension(name, ext)) {
            // Regular file with matching extension
            fileCount++;
            std::string owner = getLoginFromUID(statbuf.st_uid);
            std::cout.width(2);
            std::cout << fileCount << " : " << owner << " " << statbuf.st_size << " " << fullPath << std::endl;
        }
    }
    
    closedir(dir);
}

// Function to load user information from /etc/passwd
void loadUserInfo() {
    std::ifstream passwd("/etc/passwd");
    std::string line;
    
    if (!passwd.is_open()) {
        std::cerr << "Warning: Cannot open /etc/passwd" << std::endl;
        return;
    }
    
    while (getline(passwd, line)) {
        size_t pos = line.find(':');
        if (pos != std::string::npos) {
            std::string username = line.substr(0, pos);
            
            // Find the UID field (third field)
            size_t start = pos + 1;
            pos = line.find(':', start);
            if (pos != std::string::npos) {
                pos = line.find(':', pos + 1);
                if (pos != std::string::npos) {
                    start = line.find(':', start) + 1;
                    std::string uidStr = line.substr(start, pos - start);
                    try {
                        uid_t uid = std::stoi(uidStr);
                        uidCache[uid] = username;
                    } catch (...) {
                        // Skip if conversion fails
                    }
                }
            }
        }
    }
    
    passwd.close();
}

int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <directory> <extension>" << std::endl;
        return 1;
    }
    
    std::string dirPath = argv[1];
    std::string ext = argv[2];
    
    // Preload user info from /etc/passwd
    loadUserInfo();
    
    // Print header
    std::cout << "NO : OWNER SIZE NAME" << std::endl;
    std::cout << "-- ----- ---- ----" << std::endl;
    
    // Start recursive search
    findFiles(dirPath, ext);
    
    // Print summary
    std::cout << "+++ " << fileCount << " files match the extension " << ext << std::endl;
    
    return 0;
}