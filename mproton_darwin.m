// +build darwin

//  main.m
//  cmini
//
//  Created by Igor Okulist on 5/27/21.
//

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>

#include "mproton.h"
#include "mproton_cgo_exports.h"

// #ifdef DEBUG
#define NSLog(args...) ExtendNSLog(__FILE__, __LINE__, __PRETTY_FUNCTION__, args);
// #else
// #define NSLog(x...)
// #endif

void ExtendNSLog(const char* file, int lineNumber, const char* functionName, NSString* format, ...) {
    // Type to hold information about variable arguments.
    va_list ap;

    // Initialize a variable argument list.
    va_start(ap, format);

    // NSLog only adds a newline to the end of the NSLog format if
    // one is not already there.
    // Here we are utilizing this feature of NSLog()
    if (![format hasSuffix: @"\n"])
    {
        format = [format stringByAppendingString:@"\n"];
    }
     
    NSString *body = [[NSString alloc] initWithFormat:format arguments:ap];
     
    // End using variable argument list.
    va_end (ap);

    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    char buff[100];
    strftime(buff, sizeof buff, "%Y/%m/%d %H:%m:%S", (localtime(&ts.tv_sec)));

    NSString *fileName = [[NSString stringWithUTF8String:file] lastPathComponent];
    fprintf(stderr, "%s.%0.6ld [objc] %s " /* (%s) (%s:%d)*/ "%s",
            buff, ts.tv_nsec / 1000,
            [[NSString stringWithFormat:@"%@",[NSThread currentThread]]UTF8String],
            // functionName, [fileName UTF8String],
            // lineNumber, 
            [body UTF8String]);
}

@interface AppContext : NSObject <WKScriptMessageHandler>
@property(nonatomic, retain) NSWindow* _Nullable window;
@property(nonatomic, retain) NSMenu* _Nullable mainMenu;
@property(nonatomic, retain) NSStatusItem* _Nullable statusItem;
@property(nonatomic, retain) WKWebView* _Nullable webView;

- (void)userContentController:(nonnull WKUserContentController*)userContentController
      didReceiveScriptMessage:(nonnull WKScriptMessage*)message;

@end

@implementation AppContext

- (void)userContentController:(nonnull WKUserContentController*)userContentController
      didReceiveScriptMessage:(nonnull WKScriptMessage*)message {
    NSLog(@"int WKScriptMessageHandler callback called");

    NSDictionary* paramDict = message.body;

    // Uncomment for debugging the message body content
    // looks like "param" argument comes in as NSArrayM
    // so we have to serialize it (AFAIK)
    // for(NSString *key in [paramDict allKeys]) {
    //     NSObject * obj = [paramDict objectForKey:key];
    //     NSLog(@"%@:%@ is a %@",key, obj, [obj class]);
    // }

    NSError* writeError = nil;
    NSData* jsonData = [NSJSONSerialization dataWithJSONObject:message.body options:0 error:&writeError];
    NSString* jsonString = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];

    _prtn_call_into_go_with_reply((char*)([jsonString UTF8String]));
}
@end

AppContext* g_appContext = nil;

@interface AppDelegate : NSObject <NSApplicationDelegate>

@property(nonatomic, retain) NSWindow* _Nullable window;
@property(nonatomic, retain) NSMenu* _Nullable mainMenu;
@property(nonatomic, retain) NSStatusItem* _Nullable statusItem;

- (void)applicationWillFinishLaunching:(NSNotification*)notification;
- (void)applicationDidFinishLaunching:(NSNotification*)notification;
@end

@implementation AppDelegate

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    NSLog(@"in applicationShouldTerminateAfterLastWindowClosed");

    [NSApp stop:nil];

    return FALSE;
}

static NSWindow* createWindow(id appName) {
    NSUInteger style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable |
                       NSWindowStyleMaskResizable;
    NSWindow* window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 800, 600)
                                                   styleMask:style
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    [window cascadeTopLeftFromPoint:NSMakePoint(20, 20)];
    [window setTitle:appName];
    [window makeKeyAndOrderFront:nil];

    return window;
}

static NSMenu* createMainMenu(id appName) {
    id appMenu = [[NSMenu alloc] init];
    [appMenu addItem:[[NSMenuItem alloc] initWithTitle:[@"Quit " stringByAppendingString:appName]
                                                action:@selector(terminate:)
                                         keyEquivalent:@"q"]];

    id appMenuItem = [[NSMenuItem alloc] init];
    [appMenuItem setSubmenu:appMenu];

    NSMenu* mainMenu = [[NSMenu alloc] init];
    [mainMenu addItem:appMenuItem];

    return mainMenu;
}

static NSStatusItem* createMenuExtra() {
    NSStatusItem* statusItem =
        [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
    statusItem.button.title = @"☄️";

    NSMenu* menu = [[NSMenu alloc] init];
    [menu addItem:[NSMenuItem separatorItem]];
    [menu addItemWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@"Q"];
    statusItem.menu = menu;

    return statusItem;
}

static void registerScriptMessageHandler(WKUserContentController* userContentController, NSString* name) {
    NSString* javascript =
        [NSString stringWithFormat:
                      @"function _proton_%@_invoke(s) {  \n"
                      @"    var args = arguments;  \n"
                      @"    var promise = new Promise(function(resolve, reject) {  \n"
                      @"        var promiseId = _proton_genPromiseSequenceNumber();  \n"
                      @"        _proton_promises[promiseId] = { resolve, reject };  \n"
                      @"        window.webkit.messageHandlers.%@.postMessage({ promiseId: "
                      @"promiseId, name: \"%@\", param: Array.prototype.slice.call(args) });  \n"
                      @"    });  \n"
                      @"    return promise;  \n"
                      @"}  \n"
                      @"  \n"
                      @"window.proton['%@'] = {  \n"
                      @"    invoke: _proton_%@_invoke  \n"
                      @"}  ",
                      name,
                      name,
                      name,
                      name,
                      name];

    [userContentController
        addUserScript:[[WKUserScript alloc] initWithSource:javascript
                                             injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                                          forMainFrameOnly:YES]];
    [userContentController addScriptMessageHandler:g_appContext name:name];
}

static void executeJS(WKWebView* webView, NSString* javaScript) {
    [webView evaluateJavaScript:javaScript completionHandler:nil];
}

static WKWebView* createWebView(NSRect frame, id handler) {
    WKWebViewConfiguration* theConfiguration = [[WKWebViewConfiguration alloc] init];
    [theConfiguration.preferences setValue:@YES forKey:@"developerExtrasEnabled"];
    [theConfiguration.preferences setValue:@YES forKey:@"fullScreenEnabled"];
    [theConfiguration.preferences setValue:@YES forKey:@"javaScriptCanAccessClipboard"];
    [theConfiguration.preferences setValue:@YES forKey:@"DOMPasteAllowed"];

    WKUserContentController* userContentController = [[WKUserContentController alloc] init];
    theConfiguration.userContentController = userContentController;

    NSString* script1 =
        @"var _proton_promises = {};  "
        @"var _proton_promiseSeq = 0;  "
        @"window.proton = {};"
        @"function _proton_genPromiseSequenceNumber() {  "
        @"    _proton_promiseSeq = _proton_promiseSeq + 1;  "
        @"    return _proton_promiseSeq;  "
        @"}  "
        @"function _proton_resolvePromise(promiseId, data, error) {  "
        @"    if (error) {  "
        @"        _proton_promises[promiseId].reject(error);  "
        @"    } else {  "
        @"        _proton_promises[promiseId].resolve(data);  "
        @"    }  "
        @"    delete _proton_promises[promiseId];  "
        @"}  ";
    [userContentController
        addUserScript:[[WKUserScript alloc] initWithSource:script1
                                             injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                                          forMainFrameOnly:YES]];

    // registerScriptMessageHandler(userContentController, name);

    WKWebView* webView = [[WKWebView alloc] initWithFrame:frame configuration:theConfiguration];

    return webView;
}

- (void)applicationWillFinishLaunching:(NSNotification*)notification {
    NSLog(@"in applicationWillFinishLaunching", [NSThread currentThread]);

    id appName = [[NSProcessInfo processInfo] processName];

    self.window = g_appContext.window;

    self.mainMenu = createMainMenu(appName);
    [NSApp setMainMenu:self.mainMenu];

    self.statusItem = g_appContext.statusItem;

    self.window.contentView = g_appContext.webView;

    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    NSLog(@"in applicationDidFinishLaunching");

    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];
}

- (void)MenuExtraCallback:(NSMenuItem*)sender {
    NSString* param = [NSString 
        stringWithFormat:@"{\"name\": \"%@\", \"param\": %ld}}", @"MenuItemCallback", 
        sender.tag];

    _prtn_call_into_go_with_reply( (char*)[param UTF8String]);
}
@end

int prtn_event_loop(void) {
    @autoreleasepool {
        AppDelegate* minidelegate = [AppDelegate alloc];

        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        [NSApp setDelegate:minidelegate];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        [NSApp activateIgnoringOtherApps:YES];
        [NSApp run];
    }
    return 0;
}

#ifdef STANDALONEPROG
int main() { return prtn_event_loop(); }
#endif

int prtn_initialize(void) {
    g_appContext = [AppContext alloc];
    g_appContext.window = createWindow(@"my new name");
    g_appContext.statusItem = createMenuExtra();
    g_appContext.webView = createWebView(g_appContext.window.contentView.frame, g_appContext);
    return 0;
}

int prtn_set_title(const char* title) {
    [g_appContext.window setTitle:[NSString stringWithUTF8String:title]];

    return 0;
}

int prtn_set_menu_extra_text(const char* text) {
    g_appContext.statusItem.button.title = [NSString stringWithUTF8String:text];

    return 0;
}

int prtn_add_menu_extra_item(const char* text, int tag) {
    NSMenuItem* extractedExpr =
        [g_appContext.statusItem.menu addItemWithTitle:[NSString stringWithUTF8String:text]
                                                action:@selector(MenuExtraCallback:)
                                         keyEquivalent:@""];
    extractedExpr.tag = tag;

    return 0;
}

int prtn_set_content(const char* _Nullable content) {
    // NSLog([NSString stringWithUTF8String:path]);

    NSString* ns_content = [NSString stringWithUTF8String:content];
    // NSLog(ns_content);

    NSURL* baseUrl = [NSURL URLWithString:@""];

    [g_appContext.webView loadHTMLString:ns_content baseURL:baseUrl];

    return 0;
}

int prtn_set_content_path(const char* _Nullable path) {
    // NSLog([NSString stringWithUTF8String:path]);

    NSURL* url = [NSURL URLWithString:[NSString stringWithUTF8String:path]];
    NSString* html = [NSString stringWithContentsOfURL:url encoding:NSUTF8StringEncoding error:nil];
    NSURL* baseUrl = [NSURL URLWithString:@""];

    [g_appContext.webView loadHTMLString:html baseURL:baseUrl];

    return 0;
}

int prtn_add_script_message_handler(const char* _Nullable name) {
    // NSLog([NSString stringWithUTF8String:name]);

    NSString* ns_name = [NSString stringWithUTF8String:name];

    registerScriptMessageHandler(g_appContext.webView.configuration.userContentController, ns_name);

    return 0;
}

int prtn_execute_script(const char* _Nullable script) {
    NSLog(@"in prtn_execute_script");

    NSString* ns_script = [NSString stringWithUTF8String:script];
    dispatch_async(dispatch_get_main_queue(), ^{
      NSLog(@"in prtn_execute_script");

      executeJS(g_appContext.webView, ns_script);
    });

    NSLog(@"in prtn_execute_script block dispatched");

    return 0;
}

// p = proton.mycallback2withreply.invoke("hi").then((data)=>{console.log("done"); return data}, ()=>{console.log("bad news")})

int prtn_resolve(const char * promiseId, const char * result, const char * error) {
    NSLog(@"in prtn_resolve: %s, %s, %s\n", promiseId, result, error);

    char buf[4*1024] = "\n";
    if (error != NULL){
        sprintf(buf, "_proton_resolvePromise(%s,null, '%s');", promiseId, error);
    } else {
        sprintf(buf, "_proton_resolvePromise(%s,'%s', null);", promiseId, result);
    }

   prtn_execute_script(buf);

   return 0;
}