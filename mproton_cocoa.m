// +build darwin

//  main.m
//  cmini
//
//  Created by Igor Okulist on 5/27/21.
//

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <WebKit/WebKit.h>

#include "mproton.h"

@interface AppContext : NSObject <WKScriptMessageHandlerWithReply>
@property(nonatomic, retain)  NSWindow  * _Nullable window;
@property(nonatomic, retain)  NSMenu  * _Nullable mainMenu;
@property(nonatomic, retain)  NSStatusItem * _Nullable statusItem;
@property(nonatomic, retain)  WKWebView * _Nullable webView;

- (void)userContentController:(nonnull WKUserContentController *)userContentController
      didReceiveScriptMessage:(nonnull WKScriptMessage *)message
                 replyHandler:(nonnull void (^)(id _Nullable, NSString * _Nullable))replyHandler;
@end

@implementation AppContext
- (void)userContentController:(nonnull WKUserContentController *)userContentController
      didReceiveScriptMessage:(nonnull WKScriptMessage *)message
                 replyHandler:(nonnull void (^)(id _Nullable, NSString * _Nullable))replyHandler {
    
    NSLog(@"[objc] WKScriptMessageHandlerWithReply callback called");
    
    extern const char * goCallbackDispatcher(const void * _Nonnull, const char * _Nonnull);
    
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        struct goTrampoline_return result = goTrampoline(17, 
			(char*)([message.name UTF8String]), 
			(char*)([message.body UTF8String]));
        
        NSString * r0 = result.r0 != NULL ? [NSString stringWithUTF8String:result.r0] : NULL;
        NSString * r1 = result.r1 != NULL ? [NSString stringWithUTF8String:result.r1] : NULL;
        
        dispatch_async(dispatch_get_main_queue(), ^{
			replyHandler(r0, r1);
        });
    });
}
@end

AppContext * g_appContext = nil;


@interface AppDelegate : NSObject <
NSApplicationDelegate
>

@property(nonatomic, retain)  NSWindow  * _Nullable window;
@property(nonatomic, retain)  NSMenu  * _Nullable mainMenu;
@property(nonatomic, retain)  NSStatusItem * _Nullable statusItem;

- (void)applicationWillFinishLaunching:(NSNotification *)notification;
- (void)applicationDidFinishLaunching:(NSNotification *)notification;
@end


@implementation AppDelegate

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    NSLog(@"in applicationShouldTerminateAfterLastWindowClosed");

	[NSApp stop:nil];   
    
	return FALSE;
}

static NSWindow * createWindow(id appName) {
    NSUInteger style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable |  NSWindowStyleMaskResizable;
    NSWindow *window = [[NSWindow alloc]
                        initWithContentRect:NSMakeRect(0, 0, 800, 600)
                        styleMask:style
                        backing:NSBackingStoreBuffered
                        defer:NO];
    [window cascadeTopLeftFromPoint:NSMakePoint(20,20)];
    [window setTitle:appName];
    [window makeKeyAndOrderFront:nil];
    
    return window;
}

static NSMenu * createMainMenu(id appName) {
    id appMenu = [[NSMenu alloc] init];
    [appMenu addItem:[[NSMenuItem alloc]
                      initWithTitle:[@"Quit " stringByAppendingString:appName]
                      action:@selector(terminate:)
                      keyEquivalent:@"q"]];
    
    id appMenuItem = [[NSMenuItem alloc] init];
    [appMenuItem setSubmenu:appMenu];
    
    NSMenu *mainMenu = [[NSMenu alloc] init];
    [mainMenu addItem:appMenuItem];
    
    return mainMenu;
}

static NSStatusItem * createMenuExtra() {
    NSStatusItem * statusItem = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
    statusItem.button.title= @"🌯";
     
    NSMenu *menu = [[NSMenu alloc] init];
    [menu addItem:[NSMenuItem separatorItem]]; // A thin grey line
    [menu addItemWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@"Q"];
    statusItem.menu = menu;
    
    return statusItem;
}

static WKWebView * createWebView(NSRect frame, id handler) {
    WKWebViewConfiguration *theConfiguration = [[WKWebViewConfiguration alloc] init];
    [theConfiguration.preferences setValue:@YES forKey:@"developerExtrasEnabled"];
    [theConfiguration.preferences setValue:@YES forKey:@"fullScreenEnabled"];
    [theConfiguration.preferences setValue:@YES forKey:@"javaScriptCanAccessClipboard"];
    [theConfiguration.preferences setValue:@YES forKey:@"DOMPasteAllowed"];
    
    WKUserContentController *userContentController = [[WKUserContentController alloc] init];
    theConfiguration.userContentController = userContentController;
    
    WKWebView *webView = [[WKWebView alloc]
                          initWithFrame:frame
                          configuration:theConfiguration];

    // NSURL *nsurl=[NSURL URLWithString:@"http://www.apple.com"];
    // NSURLRequest *nsrequest=[NSURLRequest requestWithURL:nsurl];
    // [webView loadRequest:nsrequest];
    
    // NSURL *url = [NSURL URLWithString:@"file:///Users/iokulist/Github/okigan/proton2/protonappui/dist/index.html"];
    // NSString *html = [NSString stringWithContentsOfURL:url encoding:NSUTF8StringEncoding error:nil];
    // NSURL *baseUrl = [NSURL URLWithString:@""];
    
    // [webView loadHTMLString:html baseURL:baseUrl];
        
    return webView;
}

- (void) applicationWillFinishLaunching: (NSNotification *)notification {
    NSLog(@"in applicationWillFinishLaunching");
    id appName = [[NSProcessInfo processInfo] processName];
    
    self.window = g_appContext.window;
    
    self.mainMenu = createMainMenu(appName);
    [NSApp setMainMenu:self.mainMenu];
    
    self.statusItem = g_appContext.statusItem;
    
    self.window.contentView = g_appContext.webView;
    
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    NSLog(@"in applicationDidFinishLaunching");
    
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];
}

- (void)  MenuExtraCallback: (NSMenuItem*) sender {
    const char *extractedExpr = [@"p111" UTF8String];
    const char *extractedExpr2 = [@"p222" UTF8String];
    struct goTrampoline_return result = goTrampoline(17, (char*)(extractedExpr), (char*)(extractedExpr2));
    
}

@end



int xmain(void) {
    @autoreleasepool
    {
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
int main(){
    return xmain();
}
#endif

int initialize(void) {
    g_appContext = [AppContext alloc];
    g_appContext.window = createWindow(@"my new name");
    g_appContext.statusItem = createMenuExtra();
    g_appContext.webView = createWebView(g_appContext.window.contentView.frame, g_appContext);
    return 0;
}

int set_title (const char* title) {
    
    [g_appContext.window setTitle:[NSString stringWithUTF8String:title]];
    
    return 0;
}

int set_menu_extra_text (const char* text) {
    
    g_appContext.statusItem.button.title = [NSString stringWithUTF8String:text];
    
    return 0;
}

int add_menu_extra_item (const char* text) {
    
    [g_appContext.statusItem.menu
     addItemWithTitle:[NSString stringWithUTF8String:text]
     action:@selector(MenuExtraCallback:)
     keyEquivalent:@""];
    
    return 0;
}

int add_content_path (const char* _Nullable path) {
	// NSLog([NSString stringWithUTF8String:path]);

    NSURL *url = [NSURL URLWithString:[NSString stringWithUTF8String:path] ];
    NSString *html = [NSString stringWithContentsOfURL:url encoding:NSUTF8StringEncoding error:nil];
    NSURL *baseUrl = [NSURL URLWithString:@""];
    
    [g_appContext.webView loadHTMLString:html baseURL:baseUrl];
    
    return 0;
}

int add_script_message_handler(const char * _Nullable name) {
	NSString * ns_name = [NSString stringWithUTF8String:name];
    
   [g_appContext.webView.configuration.userContentController
    addScriptMessageHandlerWithReply:g_appContext
    contentWorld:WKContentWorld.pageWorld
    name:ns_name];

	return 0;
}

