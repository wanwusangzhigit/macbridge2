#include "foundation/FoundationCpp.h"
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <algorithm>

namespace WinDarling {

// ============================================================================
// NSString Implementation
// ============================================================================
NSString::NSString() : m_string("") {}

NSString::NSString(const char* str) : m_string(str ? str : "") {}

NSString::NSString(const std::string& str) : m_string(str) {}

NSString::~NSString() {}

NSString* NSString::stringWithUTF8String(const char* cString) {
    return new NSString(cString);
}

NSString* NSString::stringWithFormat(const std::string& format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format.c_str(), args);
    va_end(args);
    return new NSString(buffer);
}

const char* NSString::UTF8String() const {
    return m_string.c_str();
}

size_t NSString::length() const {
    return m_string.length();
}

bool NSString::isEqualToString(const NSString* other) const {
    return m_string == other->m_string;
}

NSString* NSString::substringFromIndex(size_t index) const {
    if (index >= m_string.length()) {
        return new NSString();
    }
    return new NSString(m_string.substr(index));
}

NSString* NSString::substringToIndex(size_t index) const {
    if (index >= m_string.length()) {
        return new NSString(m_string);
    }
    return new NSString(m_string.substr(0, index));
}

NSString* NSString::substringWithRange(size_t start, size_t length) const {
    if (start >= m_string.length()) {
        return new NSString();
    }
    size_t end = start + length;
    if (end > m_string.length()) {
        end = m_string.length();
    }
    return new NSString(m_string.substr(start, end - start));
}

// ============================================================================
// NSArray Implementation
// ============================================================================
NSArray::NSArray() : m_count(0) {}

NSArray::NSArray(size_t count) : m_count(count) {
    m_objects.reserve(count);
}

NSArray::~NSArray() {}

NSArray* NSArray::array() {
    return new NSArray();
}

NSArray* NSArray::arrayWithArray(const NSArray* other) {
    NSArray* newArray = new NSArray(other->count());
    for (size_t i = 0; i < other->count(); i++) {
        newArray->addObject(other->objectAtIndex(i));
    }
    return newArray;
}

void NSArray::addObject(void* object) {
    m_objects.push_back(object);
    m_count++;
}

void NSArray::removeObject(void* object) {
    auto it = std::find(m_objects.begin(), m_objects.end(), object);
    if (it != m_objects.end()) {
        m_objects.erase(it);
        m_count--;
    }
}

void NSArray::removeObjectAtIndex(size_t index) {
    if (index < m_count) {
        m_objects.erase(m_objects.begin() + index);
        m_count--;
    }
}

void* NSArray::objectAtIndex(size_t index) const {
    if (index < m_count) {
        return m_objects[index];
    }
    return nullptr;
}

void NSArray::insertObject(void* object, size_t index) {
    if (index <= m_count) {
        m_objects.insert(m_objects.begin() + index, object);
        m_count++;
    }
}

// ============================================================================
// NSDictionary Implementation
// ============================================================================
NSDictionary::NSDictionary() : m_count(0) {}

NSDictionary::~NSDictionary() {}

NSDictionary* NSDictionary::dictionary() {
    return new NSDictionary();
}

void NSDictionary::setObjectForKey(void* object, const NSString* key) {
    if (object && key) {
        m_objects[key->UTF8String()] = object;
        m_count++;
    }
}

void* NSDictionary::objectForKey(const NSString* key) const {
    if (key) {
        auto it = m_objects.find(key->UTF8String());
        if (it != m_objects.end()) {
            return it->second;
        }
    }
    return nullptr;
}

void NSDictionary::removeObjectForKey(const NSString* key) {
    if (key) {
        auto it = m_objects.find(key->UTF8String());
        if (it != m_objects.end()) {
            m_objects.erase(it);
            m_count--;
        }
    }
}

// ============================================================================
// NSNumber Implementation
// ============================================================================
NSNumber::NSNumber(int value) : m_type(NumberType::Int), intVal(value) {}

NSNumber::NSNumber(float value) : m_type(NumberType::Float), floatVal(value) {}

NSNumber::NSNumber(double value) : m_type(NumberType::Double), doubleVal(value) {}

NSNumber::NSNumber(bool value) : m_type(NumberType::Bool), boolVal(value) {}

NSNumber::~NSNumber() {}

int NSNumber::intValue() const {
    switch (m_type) {
        case NumberType::Int: return intVal;
        case NumberType::Float: return static_cast<int>(floatVal);
        case NumberType::Double: return static_cast<int>(doubleVal);
        case NumberType::Bool: return boolVal ? 1 : 0;
    }
    return 0;
}

float NSNumber::floatValue() const {
    switch (m_type) {
        case NumberType::Int: return static_cast<float>(intVal);
        case NumberType::Float: return floatVal;
        case NumberType::Double: return static_cast<float>(doubleVal);
        case NumberType::Bool: return boolVal ? 1.0f : 0.0f;
    }
    return 0.0f;
}

double NSNumber::doubleValue() const {
    switch (m_type) {
        case NumberType::Int: return static_cast<double>(intVal);
        case NumberType::Float: return static_cast<double>(floatVal);
        case NumberType::Double: return doubleVal;
        case NumberType::Bool: return boolVal ? 1.0 : 0.0;
    }
    return 0.0;
}

bool NSNumber::boolValue() const {
    switch (m_type) {
        case NumberType::Int: return intVal != 0;
        case NumberType::Float: return floatVal != 0.0f;
        case NumberType::Double: return doubleVal != 0.0;
        case NumberType::Bool: return boolVal;
    }
    return false;
}

// ============================================================================
// NSData Implementation
// ============================================================================
NSData::NSData() : m_bytes(nullptr), m_length(0), m_ownsMemory(false) {}

NSData::NSData(const void* bytes, size_t length) : m_length(length), m_ownsMemory(true) {
    if (bytes && length > 0) {
        m_bytes = new char[length];
        std::memcpy(m_bytes, bytes, length);
    } else {
        m_bytes = nullptr;
    }
}

NSData::~NSData() {
    if (m_ownsMemory && m_bytes) {
        delete[] static_cast<char*>(m_bytes);
    }
}

NSData* NSData::data() {
    return new NSData();
}

NSData* NSData::dataWithBytes(const void* bytes, size_t length) {
    return new NSData(bytes, length);
}

NSData* NSData::subdataWithRange(size_t offset, size_t length) const {
    if (offset >= m_length || !m_bytes) {
        return new NSData();
    }
    size_t actualLength = (offset + length > m_length) ? (m_length - offset) : length;
    return new NSData(static_cast<const char*>(m_bytes) + offset, actualLength);
}

// ============================================================================
// NSDate Implementation
// ============================================================================
NSDate::NSDate() : m_timestamp(time(nullptr)) {}

NSDate::NSDate(time_t timestamp) : m_timestamp(timestamp) {}

NSDate::~NSDate() {}

NSDate* NSDate::date() {
    return new NSDate();
}

NSDate* NSDate::dateWithTimeIntervalSinceNow(double seconds) {
    return new NSDate(time(nullptr) + static_cast<time_t>(seconds));
}

time_t NSDate::timeIntervalSince1970() const {
    return m_timestamp;
}

double NSDate::timeIntervalSinceNow() const {
    return difftime(time(nullptr), m_timestamp);
}

// ============================================================================
// NSURL Implementation
// ============================================================================
NSURL::NSURL(const std::string& urlString) : m_urlString(urlString) {
    size_t schemeEnd = urlString.find("://");
    if (schemeEnd != std::string::npos) {
        m_scheme = urlString.substr(0, schemeEnd);
        size_t hostStart = schemeEnd + 3;
        size_t pathStart = urlString.find("/", hostStart);
        if (pathStart != std::string::npos) {
            m_host = urlString.substr(hostStart, pathStart - hostStart);
            m_path = urlString.substr(pathStart);
        } else {
            m_host = urlString.substr(hostStart);
            m_path = "/";
        }
    }
}

NSURL::~NSURL() {}

NSURL* NSURL::URLWithString(const std::string& urlString) {
    return new NSURL(urlString);
}

// ============================================================================
// NSError Implementation
// ============================================================================
NSError::NSError(int code, const std::string& domain, const std::string& message)
    : m_code(code), m_domain(domain), m_message(message) {}

NSError::~NSError() {}

// ============================================================================
// NSProcessInfo Implementation
// ============================================================================
NSProcessInfo::NSProcessInfo() 
    : m_processName("WinDarling"),
      m_pid(static_cast<int>(getpid())),
      m_hostName("localhost") {}

NSProcessInfo* NSProcessInfo::processInfo() {
    static NSProcessInfo instance;
    return &instance;
}

// ============================================================================
// NSBundle Implementation
// ============================================================================
NSBundle* NSBundle::mainBundle() {
    static NSBundle* mainBundle = nullptr;
    if (!mainBundle) {
        mainBundle = new NSBundle("/usr/local/lib/windarling");
    }
    return mainBundle;
}

NSBundle* NSBundle::bundleWithPath(const std::string& path) {
    return new NSBundle(path);
}

NSBundle::NSBundle(const std::string& path)
    : m_bundlePath(path),
      m_resourcePath(path + "/Resources"),
      m_executablePath(path + "/Executable"),
      m_infoDictionary(nullptr),
      m_isLoaded(false) {}

NSBundle::~NSBundle() {
    if (m_infoDictionary) {
        delete m_infoDictionary;
    }
}

NSString* NSBundle::objectForInfoDictionaryKey(const std::string& key) {
    return new NSString("Value for " + key);
}

bool NSBundle::load() {
    m_isLoaded = true;
    return true;
}

// ============================================================================
// NSUserDefaults Implementation
// ============================================================================
NSUserDefaults* NSUserDefaults::standardUserDefaults() {
    static NSUserDefaults* defaults = nullptr;
    if (!defaults) {
        defaults = new NSUserDefaults();
    }
    return defaults;
}

NSUserDefaults::NSUserDefaults() {}

NSUserDefaults::~NSUserDefaults() {}

void NSUserDefaults::setObjectForKey(void* object, const std::string& key) {
    m_preferences[key] = object;
}

void* NSUserDefaults::objectForKey(const std::string& key) {
    auto it = m_preferences.find(key);
    return (it != m_preferences.end()) ? it->second : nullptr;
}

void NSUserDefaults::setStringForKey(const std::string& value, const std::string& key) {
    m_stringPreferences[key] = value;
}

std::string NSUserDefaults::stringForKey(const std::string& key) {
    auto it = m_stringPreferences.find(key);
    return (it != m_stringPreferences.end()) ? it->second : "";
}

void NSUserDefaults::setIntegerForKey(int value, const std::string& key) {
    m_intPreferences[key] = value;
}

int NSUserDefaults::integerForKey(const std::string& key) {
    auto it = m_intPreferences.find(key);
    return (it != m_intPreferences.end()) ? it->second : 0;
}

void NSUserDefaults::setFloatForKey(float value, const std::string& key) {
    m_floatPreferences[key] = value;
}

float NSUserDefaults::floatForKey(const std::string& key) {
    auto it = m_floatPreferences.find(key);
    return (it != m_floatPreferences.end()) ? it->second : 0.0f;
}

void NSUserDefaults::setBoolForKey(bool value, const std::string& key) {
    m_boolPreferences[key] = value;
}

bool NSUserDefaults::boolForKey(const std::string& key) {
    auto it = m_boolPreferences.find(key);
    return (it != m_boolPreferences.end()) ? it->second : false;
}

void NSUserDefaults::removeObjectForKey(const std::string& key) {
    m_preferences.erase(key);
    m_stringPreferences.erase(key);
    m_intPreferences.erase(key);
    m_floatPreferences.erase(key);
    m_boolPreferences.erase(key);
}

void NSUserDefaults::synchronize() {
    // Save preferences to disk (placeholder)
}

// ============================================================================
// NSNotificationCenter Implementation
// ============================================================================
NSNotificationCenter* NSNotificationCenter::defaultCenter() {
    static NSNotificationCenter* center = nullptr;
    if (!center) {
        center = new NSNotificationCenter();
    }
    return center;
}

NSNotificationCenter::NSNotificationCenter() {}

NSNotificationCenter::~NSNotificationCenter() {}

void NSNotificationCenter::addObserver(const std::string& name, void* observer, NotificationCallback callback) {
    ObserverInfo info;
    info.observer = observer;
    info.callback = callback;
    m_observers[name].push_back(info);
}

void NSNotificationCenter::removeObserver(const std::string& name, void* observer) {
    auto it = m_observers.find(name);
    if (it != m_observers.end()) {
        auto& observers = it->second;
        observers.erase(
            std::remove_if(observers.begin(), observers.end(),
                          [observer](const ObserverInfo& info) {
                              return info.observer == observer;
                          }),
            observers.end()
        );
    }
}

void NSNotificationCenter::removeObserver(void* observer) {
    for (auto& pair : m_observers) {
        auto& observers = pair.second;
        observers.erase(
            std::remove_if(observers.begin(), observers.end(),
                          [observer](const ObserverInfo& info) {
                              return info.observer == observer;
                          }),
            observers.end()
        );
    }
}

void NSNotificationCenter::postNotificationName(const std::string& name, void* object) {
    auto it = m_observers.find(name);
    if (it != m_observers.end()) {
        for (auto& info : it->second) {
            if (info.callback) {
                info.callback(object);
            }
        }
    }
}

// ============================================================================
// NSTimer Implementation
// ============================================================================
NSTimer* NSTimer::scheduledTimerWithTimeInterval(double seconds, 
                                                   bool repeats,
                                                   std::function<void(NSTimer*)> block) {
    return new NSTimer(seconds, repeats, block);
}

NSTimer::NSTimer(double seconds, bool repeats, std::function<void(NSTimer*)> block)
    : m_timeInterval(seconds),
      m_repeats(repeats),
      m_valid(true),
      m_block(block) {}

NSTimer::~NSTimer() {}

void NSTimer::fire() {
    if (m_valid && m_block) {
        m_block(this);
        if (!m_repeats) {
            m_valid = false;
        }
    }
}

void NSTimer::invalidate() {
    m_valid = false;
}

// ============================================================================
// NSFileManager Implementation
// ============================================================================
NSFileManager* NSFileManager::defaultManager() {
    static NSFileManager* manager = nullptr;
    if (!manager) {
        manager = new NSFileManager();
    }
    return manager;
}

NSFileManager::NSFileManager() {}

NSFileManager::~NSFileManager() {}

bool NSFileManager::fileExistsAtPath(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool NSFileManager::isDirectoryAtPath(const std::string& path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) == 0) {
        return S_ISDIR(buffer.st_mode);
    }
    return false;
}

bool NSFileManager::createDirectoryAtPath(const std::string& path, bool withIntermediateDirectories) {
    if (withIntermediateDirectories) {
        return (mkdir(path.c_str(), 0755) == 0 || errno == EEXIST);
    }
    return (mkdir(path.c_str(), 0755) == 0);
}

bool NSFileManager::removeItemAtPath(const std::string& path) {
    return (remove(path.c_str()) == 0);
}

bool NSFileManager::copyItemAtPath(const std::string& srcPath, const std::string& dstPath) {
    // Simplified implementation
    return false;
}

bool NSFileManager::moveItemAtPath(const std::string& srcPath, const std::string& dstPath) {
    return (rename(srcPath.c_str(), dstPath.c_str()) == 0);
}

NSDictionary* NSFileManager::attributesOfItemAtPath(const std::string& path) {
    return NSDictionary::dictionary();
}

NSArray* NSFileManager::contentsOfDirectoryAtPath(const std::string& path) {
    NSArray* result = NSArray::array();
    DIR* dir = opendir(path.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_name[0] != '.') {
                result->addObject(new NSString(entry->d_name));
            }
        }
        closedir(dir);
    }
    return result;
}

NSData* NSFileManager::contentsAtPath(const std::string& path) {
    FILE* file = fopen(path.c_str(), "rb");
    if (!file) return nullptr;
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* buffer = new char[size];
    fread(buffer, 1, size, file);
    fclose(file);
    
    NSData* data = new NSData(buffer, size);
    delete[] buffer;
    return data;
}

bool NSFileManager::createFileAtPath(const std::string& path, NSData* contents, NSDictionary* attributes) {
    FILE* file = fopen(path.c_str(), "wb");
    if (!file) return false;
    
    if (contents && contents->bytes()) {
        fwrite(contents->bytes(), 1, contents->length(), file);
    }
    fclose(file);
    return true;
}

std::string NSFileManager::currentDirectoryPath() {
    char cwd[1024];
    return getcwd(cwd, sizeof(cwd)) ? std::string(cwd) : "";
}

bool NSFileManager::changeCurrentDirectoryPath(const std::string& path) {
    return (chdir(path.c_str()) == 0);
}

// ============================================================================
// NSLock Implementation
// ============================================================================
NSLock::NSLock() : m_locked(false), m_mutex(nullptr) {
    pthread_mutex_t* mutex = new pthread_mutex_t;
    pthread_mutex_init(mutex, nullptr);
    m_mutex = mutex;
}

NSLock::~NSLock() {
    if (m_mutex) {
        pthread_mutex_destroy(static_cast<pthread_mutex_t*>(m_mutex));
        delete static_cast<pthread_mutex_t*>(m_mutex);
    }
}

void NSLock::lock() {
    if (m_mutex) {
        pthread_mutex_lock(static_cast<pthread_mutex_t*>(m_mutex));
        m_locked = true;
    }
}

void NSLock::unlock() {
    if (m_mutex) {
        m_locked = false;
        pthread_mutex_unlock(static_cast<pthread_mutex_t*>(m_mutex));
    }
}

bool NSLock::tryLock() {
    if (m_mutex) {
        int result = pthread_mutex_trylock(static_cast<pthread_mutex_t*>(m_mutex));
        if (result == 0) {
            m_locked = true;
            return true;
        }
    }
    return false;
}

// ============================================================================
// NSThread Implementation
// ============================================================================
NSThread::NSThread() 
    : m_target(nullptr),
      m_executing(false),
      m_finished(false),
      m_cancelled(false),
      m_pthread(nullptr) {}

NSThread::NSThread(std::function<void()> target)
    : m_target(target),
      m_executing(false),
      m_finished(false),
      m_cancelled(false),
      m_pthread(nullptr) {}

NSThread::~NSThread() {}

void NSThread::start() {
    if (!m_executing && m_target) {
        m_executing = true;
        m_finished = false;
        m_target();
        m_executing = false;
        m_finished = true;
    }
}

void NSThread::cancel() {
    m_cancelled = true;
}

NSThread* NSThread::currentThread() {
    return nullptr; // Placeholder
}

void NSThread::sleepForTimeInterval(double seconds) {
    usleep(static_cast<useconds_t>(seconds * 1000000));
}

} // namespace WinDarling
