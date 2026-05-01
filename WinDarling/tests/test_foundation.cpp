#include <iostream>
#include <cassert>
#include <cstring>
#include "foundation/FoundationCpp.h"

using namespace WinDarling;

void testNSString() {
    std::cout << "Testing NSString..." << std::endl;
    
    NSString* str = NSString::stringWithUTF8String("Hello, World!");
    assert(str != nullptr);
    assert(str->length() == 13);
    std::cout << "  ✓ NSString creation and length" << std::endl;
    
    NSString* sub = str->substringFromIndex(7);
    assert(sub != nullptr);
    std::cout << "  ✓ NSString substring" << std::endl;
    
    delete str;
    delete sub;
    std::cout << "NSString tests passed!\n" << std::endl;
}

void testNSArray() {
    std::cout << "Testing NSArray..." << std::endl;
    
    NSArray* array = NSArray::array();
    assert(array != nullptr);
    assert(array->count() == 0);
    
    array->addObject(new NSString("Item 1"));
    array->addObject(new NSString("Item 2"));
    assert(array->count() == 2);
    std::cout << "  ✓ NSArray add and count" << std::endl;
    
    void* obj = array->objectAtIndex(0);
    assert(obj != nullptr);
    std::cout << "  ✓ NSArray objectAtIndex" << std::endl;
    
    delete array;
    std::cout << "NSArray tests passed!\n" << std::endl;
}

void testNSDictionary() {
    std::cout << "Testing NSDictionary..." << std::endl;
    
    NSDictionary* dict = NSDictionary::dictionary();
    assert(dict != nullptr);
    
    NSString* key = NSString::stringWithUTF8String("testKey");
    NSString* value = NSString::stringWithUTF8String("testValue");
    dict->setObjectForKey(value, key);
    
    void* retrieved = dict->objectForKey(key);
    assert(retrieved != nullptr);
    std::cout << "  ✓ NSDictionary setObjectForKey" << std::endl;
    
    delete dict;
    delete key;
    std::cout << "NSDictionary tests passed!\n" << std::endl;
}

void testNSNumber() {
    std::cout << "Testing NSNumber..." << std::endl;
    
    NSNumber* intNum = new NSNumber(42);
    assert(intNum->intValue() == 42);
    std::cout << "  ✓ NSNumber int" << std::endl;
    
    NSNumber* floatNum = new NSNumber(3.14f);
    assert(floatNum->floatValue() > 3.0f);
    std::cout << "  ✓ NSNumber float" << std::endl;
    
    NSNumber* boolNum = new NSNumber(true);
    assert(boolNum->boolValue() == true);
    std::cout << "  ✓ NSNumber bool" << std::endl;
    
    delete intNum;
    delete floatNum;
    delete boolNum;
    std::cout << "NSNumber tests passed!\n" << std::endl;
}

void testNSData() {
    std::cout << "Testing NSData..." << std::endl;
    
    const char* testData = "Test data";
    NSData* data = NSData::dataWithBytes(testData, std::strlen(testData));
    assert(data != nullptr);
    assert(data->length() == std::strlen(testData));
    std::cout << "  ✓ NSData creation and length" << std::endl;
    
    delete data;
    std::cout << "NSData tests passed!\n" << std::endl;
}

void testNSDate() {
    std::cout << "Testing NSDate..." << std::endl;
    
    NSDate* date = NSDate::date();
    assert(date != nullptr);
    assert(date->timeIntervalSince1970() > 0);
    std::cout << "  ✓ NSDate creation" << std::endl;
    
    delete date;
    std::cout << "NSDate tests passed!\n" << std::endl;
}

void testNSURL() {
    std::cout << "Testing NSURL..." << std::endl;
    
    NSURL* url = NSURL::URLWithString("https://example.com/path");
    assert(url != nullptr);
    assert(url->scheme() == "https");
    assert(url->host() == "example.com");
    assert(url->path() == "/path");
    std::cout << "  ✓ NSURL parsing" << std::endl;
    
    delete url;
    std::cout << "NSURL tests passed!\n" << std::endl;
}

void testNSUserDefaults() {
    std::cout << "Testing NSUserDefaults..." << std::endl;
    
    NSUserDefaults* defaults = NSUserDefaults::standardUserDefaults();
    assert(defaults != nullptr);
    
    defaults->setStringForKey("Hello", "testString");
    assert(defaults->stringForKey("testString") == "Hello");
    std::cout << "  ✓ NSUserDefaults string" << std::endl;
    
    defaults->setIntegerForKey(42, "testInt");
    assert(defaults->integerForKey("testInt") == 42);
    std::cout << "  ✓ NSUserDefaults integer" << std::endl;
    
    defaults->setBoolForKey(true, "testBool");
    assert(defaults->boolForKey("testBool") == true);
    std::cout << "  ✓ NSUserDefaults bool" << std::endl;
    
    std::cout << "NSUserDefaults tests passed!\n" << std::endl;
}

void testNSNotificationCenter() {
    std::cout << "Testing NSNotificationCenter..." << std::endl;
    
    NSNotificationCenter* center = NSNotificationCenter::defaultCenter();
    assert(center != nullptr);
    
    bool notificationReceived = false;
    void* observer = reinterpret_cast<void*>(0x123456);
    
    center->addObserver("TestNotification", observer, [&notificationReceived](void* obj) {
        notificationReceived = true;
    });
    
    center->postNotificationName("TestNotification");
    assert(notificationReceived == true);
    std::cout << "  ✓ NSNotificationCenter post and receive" << std::endl;
    
    center->removeObserver("TestNotification", observer);
    std::cout << "  ✓ NSNotificationCenter remove observer" << std::endl;
    
    std::cout << "NSNotificationCenter tests passed!\n" << std::endl;
}

void testNSFileManager() {
    std::cout << "Testing NSFileManager..." << std::endl;
    
    NSFileManager* manager = NSFileManager::defaultManager();
    assert(manager != nullptr);
    
    std::string cwd = manager->currentDirectoryPath();
    assert(!cwd.empty());
    std::cout << "  ✓ NSFileManager currentDirectory: " << cwd << std::endl;
    
    bool exists = manager->fileExistsAtPath("/tmp");
    assert(exists == true);
    std::cout << "  ✓ NSFileManager fileExistsAtPath" << std::endl;
    
    std::cout << "NSFileManager tests passed!\n" << std::endl;
}

void testNSLock() {
    std::cout << "Testing NSLock..." << std::endl;
    
    NSLock* lock = new NSLock();
    assert(lock != nullptr);
    assert(!lock->isLocked());
    
    lock->lock();
    assert(lock->isLocked());
    std::cout << "  ✓ NSLock lock" << std::endl;
    
    lock->unlock();
    assert(!lock->isLocked());
    std::cout << "  ✓ NSLock unlock" << std::endl;
    
    delete lock;
    std::cout << "NSLock tests passed!\n" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  WinDarling Foundation Test Suite" << std::endl;
    std::cout << "========================================" << std::endl << std::endl;
    
    try {
        testNSString();
        testNSArray();
        testNSDictionary();
        testNSNumber();
        testNSData();
        testNSDate();
        testNSURL();
        testNSUserDefaults();
        testNSNotificationCenter();
        testNSFileManager();
        testNSLock();
        
        std::cout << "========================================" << std::endl;
        std::cout << "  All Foundation tests passed!" << std::endl;
        std::cout << "========================================" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}
