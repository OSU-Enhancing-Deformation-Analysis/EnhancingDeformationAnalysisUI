#import <Cocoa/Cocoa.h>
#include "utils.h"

namespace utils {

std::string OpenFileDialog(
    const char* open_path,
    const char* title,
    bool folders_only,
    const char* filter
) {
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];

        if (open_path && open_path[0]) {
            NSString* start = [NSString stringWithUTF8String:open_path];
            NSURL* url = [NSURL fileURLWithPath:start];
            [panel setDirectoryURL:url];
        }

        if (title && title[0]) {
            NSString* t = [NSString stringWithUTF8String:title];
            [panel setTitle:t];
        }

        if (folders_only) {
            [panel setCanChooseFiles:NO];
            [panel setCanChooseDirectories:YES];
        } else {
            [panel setCanChooseFiles:YES];
            [panel setCanChooseDirectories:NO];
        }

        [panel setAllowsMultipleSelection:NO];

        if (!folders_only && filter && filter[0]) {
            NSString* f = [NSString stringWithUTF8String:filter];
            [panel setAllowedFileTypes:@[f]];
        }

        if ([panel runModal] == NSModalResponseOK) {
            NSURL* url = [[panel URLs] objectAtIndex:0];
            return std::string([[url path] UTF8String]);
        }

        return {};
    }
}

} // namespace utils
