#ifndef FOUNDATION_CPP_H
#define FOUNDATION_CPP_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <functional>
#include <memory>

namespace WinDarling {

// ============================================================================
// NSString
// ============================================================================
class NSString {
public:
    NSString();
    NSString(const char* str);
    NSString(const std::string& str);
    ~NSString();
    
    static NSString* stringWithUTF8String(const char* cString);
    static NSString* stringWithFormat(const std::string& format, ...);
    
    const char* UTF8String() const;
    size_t length() const;
    bool isEqualToString(const NSString* other) const;
    
    NSString* substringFromIndex(size_t index) const;
    NSString* substringToIndex(size_t index) const;
    NSString* substringWithRange(size_t start, size_t length) const;
    
private:
    std::string m_string;
};

// ============================================================================
// NSArray
// ============================================================================
class NSArray {
public:
    NSArray();
    explicit NSArray(size_t count);
    ~NSArray();
    
    static NSArray* array();
    static NSArray* arrayWithArray(const NSArray* other);
    
    size_t count() const { return m_count; }
    void addObject(void* object);
    void removeObject(void* object);
    void removeObjectAtIndex(size_t index);
    void* objectAtIndex(size_t index) const;
    void insertObject(void* object, size_t index);
    
private:
    std::vector<void*> m_objects;
    size_t m_count;
};

// ============================================================================
// NSDictionary
// ============================================================================
class NSDictionary {
public:
    NSDictionary();
    ~NSDictionary();
    
    static NSDictionary* dictionary();
    
    size_t count() const { return m_count; }
    void setObjectForKey(void* object, const NSString* key);
    void* objectForKey(const NSString* key) const;
    void removeObjectForKey(const NSString* key);
    
private:
    std::map<std::string, void*> m_objects;
    size_t m_count;
};

// ============================================================================
// NSNumber
// ============================================================================
class NSNumber {
public:
    NSNumber(int value);
    NSNumber(float value);
    NSNumber(double value);
    NSNumber(bool value);
    ~NSNumber();
    
    int intValue() const;
    float floatValue() const;
    double doubleValue() const;
    bool boolValue() const;
    
private:
    enum class NumberType { Int, Float, Double, Bool };
    NumberType m_type;
    union {
        int intVal;
        float floatVal;
        double doubleVal;
        bool boolVal;
    };
};

// ============================================================================
// NSData
// ============================================================================
class NSData {
public:
    NSData();
    NSData(const void* bytes, size_t length);
    ~NSData();
    
    static NSData* data();
    static NSData* dataWithBytes(const void* bytes, size_t length);
    
    size_t length() const { return m_length; }
    const void* bytes() const { return m_bytes; }
    
    NSData* subdataWithRange(size_t offset, size_t length) const;
    
private:
    void* m_bytes;
    size_t m_length;
    bool m_ownsMemory;
};

// ============================================================================
// NSDate
// ============================================================================
class NSDate {
public:
    NSDate();
    explicit NSDate(time_t timestamp);
    ~NSDate();
    
    static NSDate* date();
    static NSDate* dateWithTimeIntervalSinceNow(double seconds);
    
    time_t timeIntervalSince1970() const;
    double timeIntervalSinceNow() const;
    
private:
    time_t m_timestamp;
};

// ============================================================================
// NSURL
// ============================================================================
class NSURL {
public:
    NSURL(const std::string& urlString);
    ~NSURL();
    
    static NSURL* URLWithString(const std::string& urlString);
    
    std::string absoluteString() const { return m_urlString; }
    std::string scheme() const { return m_scheme; }
    std::string host() const { return m_host; }
    std::string path() const { return m_path; }
    
private:
    std::string m_urlString;
    std::string m_scheme;
    std::string m_host;
    std::string m_path;
};

// ============================================================================
// NSError
// ============================================================================
class NSError {
public:
    NSError(int code, const std::string& domain, const std::string& message);
    ~NSError();
    
    int getErrorCode() const { return m_code; }
    std::string getErrorDomain() const { return m_domain; }
    std::string getErrorMessage() const { return m_message; }
    
private:
    int m_code;
    std::string m_domain;
    std::string m_message;
};

// ============================================================================
// NSProcessInfo
// ============================================================================
class NSProcessInfo {
public:
    static NSProcessInfo* processInfo();
    
    std::string getProcessName() const { return m_processName; }
    void setProcessName(const std::string& name) { m_processName = name; }
    
    int getProcessIdentifier() const { return m_pid; }
    
    std::string getHostName() const { return m_hostName; }
    
private:
    NSProcessInfo();
    std::string m_processName;
    int m_pid;
    std::string m_hostName;
};

// ============================================================================
// NSBundle - NEW!
// ============================================================================
class NSBundle {
public:
    static NSBundle* mainBundle();
    static NSBundle* bundleWithPath(const std::string& path);
    
    ~NSBundle();
    
    std::string bundlePath() const { return m_bundlePath; }
    std::string resourcePath() const { return m_resourcePath; }
    std::string executablePath() const { return m_executablePath; }
    
    NSString* objectForInfoDictionaryKey(const std::string& key);
    NSDictionary* infoDictionary() const { return m_infoDictionary; }
    
    bool load();
    bool isLoaded() const { return m_isLoaded; }
    
private:
    NSBundle(const std::string& path);
    
    std::string m_bundlePath;
    std::string m_resourcePath;
    std::string m_executablePath;
    NSDictionary* m_infoDictionary;
    bool m_isLoaded;
};

// ============================================================================
// NSUserDefaults - NEW!
// ============================================================================
class NSUserDefaults {
public:
    static NSUserDefaults* standardUserDefaults();
    ~NSUserDefaults();
    
    void setObjectForKey(void* object, const std::string& key);
    void* objectForKey(const std::string& key);
    
    void setStringForKey(const std::string& value, const std::string& key);
    std::string stringForKey(const std::string& key);
    
    void setIntegerForKey(int value, const std::string& key);
    int integerForKey(const std::string& key);
    
    void setFloatForKey(float value, const std::string& key);
    float floatForKey(const std::string& key);
    
    void setBoolForKey(bool value, const std::string& key);
    bool boolForKey(const std::string& key);
    
    void removeObjectForKey(const std::string& key);
    
    void synchronize();
    
private:
    NSUserDefaults();
    
    std::map<std::string, void*> m_preferences;
    std::map<std::string, std::string> m_stringPreferences;
    std::map<std::string, int> m_intPreferences;
    std::map<std::string, float> m_floatPreferences;
    std::map<std::string, bool> m_boolPreferences;
};

// ============================================================================
// NSNotificationCenter - NEW!
// ============================================================================
class NSNotificationCenter {
public:
    static NSNotificationCenter* defaultCenter();
    ~NSNotificationCenter();
    
    typedef std::function<void(void*)> NotificationCallback;
    
    void addObserver(const std::string& name, void* observer, NotificationCallback callback);
    void removeObserver(const std::string& name, void* observer);
    void removeObserver(void* observer);
    
    void postNotificationName(const std::string& name, void* object = nullptr);
    
    size_t observerCount() const { return m_observers.size(); }
    
private:
    NSNotificationCenter();
    
    struct ObserverInfo {
        void* observer;
        NotificationCallback callback;
    };
    
    std::map<std::string, std::vector<ObserverInfo>> m_observers;
};

// ============================================================================
// NSTimer - NEW!
// ============================================================================
class NSTimer {
public:
    static NSTimer* scheduledTimerWithTimeInterval(double seconds, 
                                                     bool repeats,
                                                     std::function<void(NSTimer*)> block);
    
    ~NSTimer();
    
    void fire();
    void invalidate();
    bool isValid() const { return m_valid; }
    
    double timeInterval() const { return m_timeInterval; }
    bool repeats() const { return m_repeats; }
    
private:
    NSTimer(double seconds, bool repeats, std::function<void(NSTimer*)> block);
    
    double m_timeInterval;
    bool m_repeats;
    bool m_valid;
    std::function<void(NSTimer*)> m_block;
};

// ============================================================================
// NSFileManager - NEW!
// ============================================================================
class NSFileManager {
public:
    static NSFileManager* defaultManager();
    ~NSFileManager();
    
    bool fileExistsAtPath(const std::string& path);
    bool isDirectoryAtPath(const std::string& path);
    
    bool createDirectoryAtPath(const std::string& path, bool withIntermediateDirectories);
    bool removeItemAtPath(const std::string& path);
    bool copyItemAtPath(const std::string& srcPath, const std::string& dstPath);
    bool moveItemAtPath(const std::string& srcPath, const std::string& dstPath);
    
    NSDictionary* attributesOfItemAtPath(const std::string& path);
    
    NSArray* contentsOfDirectoryAtPath(const std::string& path);
    NSData* contentsAtPath(const std::string& path);
    bool createFileAtPath(const std::string& path, NSData* contents, NSDictionary* attributes);
    
    std::string currentDirectoryPath();
    bool changeCurrentDirectoryPath(const std::string& path);
    
private:
    NSFileManager();
};

// ============================================================================
// NSLock - NEW!
// ============================================================================
class NSLock {
public:
    NSLock();
    ~NSLock();
    
    void lock();
    void unlock();
    bool tryLock();
    
    bool isLocked() const { return m_locked; }
    
private:
    bool m_locked;
    void* m_mutex;
};

// ============================================================================
// NSThread - NEW!
// ============================================================================
class NSThread {
public:
    NSThread();
    explicit NSThread(std::function<void()> target);
    ~NSThread();
    
    void start();
    void cancel();
    bool isExecuting() const { return m_executing; }
    bool isFinished() const { return m_finished; }
    bool isCancelled() const { return m_cancelled; }
    
    static NSThread* currentThread();
    static void sleepForTimeInterval(double seconds);
    
    void setName(const std::string& name) { m_name = name; }
    std::string name() const { return m_name; }
    
private:
    std::function<void()> m_target;
    bool m_executing;
    bool m_finished;
    bool m_cancelled;
    std::string m_name;
    void* m_pthread;
};

} // namespace WinDarling

#endif // FOUNDATION_CPP_H
