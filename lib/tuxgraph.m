/*
 * TuxGraph - Graphics bridge library for TuxPascal
 * Provides simple pixel graphics using macOS Core Graphics and AppKit
 * Now with retro sound effects!
 */

#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>
#import <AudioToolbox/AudioToolbox.h>
#import <math.h>

// Global state
static NSWindow *gWindow = nil;
static NSView *gView = nil;
static uint32_t *gFramebuffer = nil;
static int gWidth = 0;
static int gHeight = 0;
static BOOL gRunning = YES;

// Key buffer for non-blocking key reads
#define KEY_BUFFER_SIZE 32
static int gKeyBuffer[KEY_BUFFER_SIZE];
static int gKeyHead = 0;
static int gKeyTail = 0;

static void pushKey(int key) {
    int next = (gKeyHead + 1) % KEY_BUFFER_SIZE;
    if (next != gKeyTail) {  // Buffer not full
        gKeyBuffer[gKeyHead] = key;
        gKeyHead = next;
    }
}

static int popKey(void) {
    if (gKeyHead == gKeyTail) return -1;  // Buffer empty
    int key = gKeyBuffer[gKeyTail];
    gKeyTail = (gKeyTail + 1) % KEY_BUFFER_SIZE;
    return key;
}

// ============================================================
// Sound System using AudioQueue
// ============================================================

#define SAMPLE_RATE 22050  // Lower sample rate for retro feel
#define NUM_BUFFERS 3
#define BUFFER_SIZE 1024   // ~23ms per buffer - good balance

static AudioQueueRef gAudioQueue = NULL;
static AudioQueueBufferRef gAudioBuffers[NUM_BUFFERS];
static int gSoundFrequency = 0;
static int gSoundDuration = 0;      // in samples
static int gSoundPosition = 0;
static int gSoundWaveform = 0;      // 0=square, 1=sine, 2=noise
static int gSoundVolume = 100;      // 0-100
static BOOL gSoundPlaying = NO;

// LCG random for noise
static uint32_t gNoiseState = 12345;
static int noiseRandom(void) {
    gNoiseState = gNoiseState * 1103515245 + 12345;
    return (gNoiseState >> 16) & 0x7FFF;
}

static void audioCallback(void *userData, AudioQueueRef queue, AudioQueueBufferRef buffer) {
    int16_t *samples = (int16_t *)buffer->mAudioData;
    int numSamples = BUFFER_SIZE / sizeof(int16_t);
    int fadeLen = SAMPLE_RATE / 200;  // 5ms fade

    for (int i = 0; i < numSamples; i++) {
        if (gSoundPlaying && gSoundPosition < gSoundDuration) {
            double t = (double)gSoundPosition / SAMPLE_RATE;
            double amplitude = 12000.0 * gSoundVolume / 100.0;

            // Fade in/out to avoid clicks
            if (gSoundPosition < fadeLen) {
                amplitude *= (double)gSoundPosition / fadeLen;
            } else if (gSoundPosition > gSoundDuration - fadeLen) {
                amplitude *= (double)(gSoundDuration - gSoundPosition) / fadeLen;
            }

            int16_t sample = 0;
            if (gSoundWaveform == 0) {
                // Square wave (softer duty cycle for less harshness)
                double period = 1.0 / gSoundFrequency;
                double phase = fmod(t, period) / period;
                sample = (phase < 0.4) ? (int16_t)amplitude : (int16_t)(-amplitude);
            } else if (gSoundWaveform == 1) {
                // Sine wave
                sample = (int16_t)(amplitude * sin(2.0 * M_PI * gSoundFrequency * t));
            } else {
                // Noise
                sample = (int16_t)((noiseRandom() - 16384) * gSoundVolume / 100);
            }

            samples[i] = sample;
            gSoundPosition++;
        } else {
            samples[i] = 0;
            gSoundPlaying = NO;
        }
    }

    buffer->mAudioDataByteSize = BUFFER_SIZE;
    AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
}

static void initAudio(void) {
    if (gAudioQueue) return;  // Already initialized

    AudioStreamBasicDescription format = {0};
    format.mSampleRate = SAMPLE_RATE;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
    format.mBitsPerChannel = 16;
    format.mChannelsPerFrame = 1;
    format.mBytesPerFrame = 2;
    format.mFramesPerPacket = 1;
    format.mBytesPerPacket = 2;

    OSStatus status = AudioQueueNewOutput(&format, audioCallback, NULL,
                                          CFRunLoopGetCurrent(), kCFRunLoopCommonModes,
                                          0, &gAudioQueue);
    if (status != noErr) return;

    for (int i = 0; i < NUM_BUFFERS; i++) {
        AudioQueueAllocateBuffer(gAudioQueue, BUFFER_SIZE, &gAudioBuffers[i]);
        gAudioBuffers[i]->mAudioDataByteSize = BUFFER_SIZE;
        memset(gAudioBuffers[i]->mAudioData, 0, BUFFER_SIZE);
        AudioQueueEnqueueBuffer(gAudioQueue, gAudioBuffers[i], 0, NULL);
    }

    AudioQueueStart(gAudioQueue, NULL);
}

static void cleanupAudio(void) {
    if (gAudioQueue) {
        AudioQueueStop(gAudioQueue, true);
        AudioQueueDispose(gAudioQueue, true);
        gAudioQueue = NULL;
    }
}

// Custom view that displays our framebuffer and handles keys
@interface TuxGraphView : NSView
@end

@implementation TuxGraphView

- (void)drawRect:(NSRect)dirtyRect {
    if (gFramebuffer && gWidth > 0 && gHeight > 0) {
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef ctx = CGBitmapContextCreate(
            gFramebuffer,
            gWidth, gHeight,
            8,                          // bits per component
            gWidth * 4,                 // bytes per row
            colorSpace,
            kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little
        );

        CGImageRef image = CGBitmapContextCreateImage(ctx);

        NSGraphicsContext *nsCtx = [NSGraphicsContext currentContext];
        CGContextRef drawCtx = [nsCtx CGContext];

        // Flip vertically (Core Graphics origin is bottom-left)
        CGContextTranslateCTM(drawCtx, 0, self.bounds.size.height);
        CGContextScaleCTM(drawCtx, 1.0, -1.0);

        CGContextDrawImage(drawCtx, CGRectMake(0, 0, gWidth, gHeight), image);

        CGImageRelease(image);
        CGContextRelease(ctx);
        CGColorSpaceRelease(colorSpace);
    }
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)canBecomeKeyView {
    return YES;
}

- (void)keyDown:(NSEvent *)event {
    // Handle key press - don't call super to prevent beep
    NSString *chars = [event charactersIgnoringModifiers];
    if ([chars length] > 0) {
        unichar ch = [chars characterAtIndex:0];
        pushKey((int)ch);
    }
}

- (void)keyUp:(NSEvent *)event {
    // Absorb key up to prevent beep
}

@end

// Window delegate to handle close
@interface TuxGraphDelegate : NSObject <NSWindowDelegate>
@end

@implementation TuxGraphDelegate

- (BOOL)windowShouldClose:(NSWindow *)sender {
    gRunning = NO;
    return YES;
}

@end

static TuxGraphDelegate *gDelegate = nil;

// ============================================================
// Pascal-callable functions (C interface)
// ============================================================

// Initialize graphics window
// Returns: 1 on success, 0 on failure
int gfx_init(int width, int height) {
    @autoreleasepool {
        // Initialize the application
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        // Create framebuffer
        gWidth = width;
        gHeight = height;
        gFramebuffer = (uint32_t *)malloc(width * height * sizeof(uint32_t));
        if (!gFramebuffer) return 0;

        // Clear to opaque black (alpha = 0xFF)
        for (int i = 0; i < width * height; i++) {
            gFramebuffer[i] = 0xFF000000;
        }

        // Create window
        NSRect frame = NSMakeRect(100, 100, width, height);
        gWindow = [[NSWindow alloc]
            initWithContentRect:frame
            styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable
            backing:NSBackingStoreBuffered
            defer:NO];

        [gWindow setTitle:@"TuxPascal Graphics"];

        // Create custom view
        gView = [[TuxGraphView alloc] initWithFrame:NSMakeRect(0, 0, width, height)];
        [gWindow setContentView:gView];

        // Set up delegate
        gDelegate = [[TuxGraphDelegate alloc] init];
        [gWindow setDelegate:gDelegate];

        // Show window and make view first responder for key events
        [gWindow makeKeyAndOrderFront:nil];
        [gWindow makeFirstResponder:gView];
        [NSApp activateIgnoringOtherApps:YES];

        // Clear key buffer
        gKeyHead = 0;
        gKeyTail = 0;

        gRunning = YES;
        return 1;
    }
}

// Close graphics window and cleanup
void gfx_close(void) {
    @autoreleasepool {
        cleanupAudio();  // Stop sound system
        if (gFramebuffer) {
            free(gFramebuffer);
            gFramebuffer = nil;
        }
        if (gWindow) {
            [gWindow close];
            gWindow = nil;
        }
        gRunning = NO;
    }
}

// Set a single pixel (x, y, color as 0xRRGGBB)
// Y is flipped so y=0 is at top (standard screen coordinates)
void gfx_set_pixel(int x, int y, int color) {
    if (gFramebuffer && x >= 0 && x < gWidth && y >= 0 && y < gHeight) {
        int flippedY = gHeight - 1 - y;  // Flip Y axis
        gFramebuffer[flippedY * gWidth + x] = 0xFF000000 | (color & 0xFFFFFF);
    }
}

// Set pixel with separate RGB components
void gfx_set_pixel_rgb(int x, int y, int r, int g, int b) {
    if (gFramebuffer && x >= 0 && x < gWidth && y >= 0 && y < gHeight) {
        int flippedY = gHeight - 1 - y;  // Flip Y axis
        uint32_t color = 0xFF000000 | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
        gFramebuffer[flippedY * gWidth + x] = color;
    }
}

// Get pixel color at (x, y), returns 0xRRGGBB
int gfx_get_pixel(int x, int y) {
    if (gFramebuffer && x >= 0 && x < gWidth && y >= 0 && y < gHeight) {
        int flippedY = gHeight - 1 - y;  // Flip Y axis
        return gFramebuffer[flippedY * gWidth + x] & 0xFFFFFF;
    }
    return 0;
}

// Clear screen to a color (0xRRGGBB)
void gfx_clear(int color) {
    if (gFramebuffer) {
        uint32_t c = 0xFF000000 | (color & 0xFFFFFF);
        for (int i = 0; i < gWidth * gHeight; i++) {
            gFramebuffer[i] = c;
        }
    }
}

// Draw a line from (x1,y1) to (x2,y2) using Bresenham's algorithm
void gfx_line(int x1, int y1, int x2, int y2, int color) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        gfx_set_pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}

// Draw a rectangle outline
void gfx_rect(int x, int y, int w, int h, int color) {
    gfx_line(x, y, x + w - 1, y, color);           // Top
    gfx_line(x, y + h - 1, x + w - 1, y + h - 1, color); // Bottom
    gfx_line(x, y, x, y + h - 1, color);           // Left
    gfx_line(x + w - 1, y, x + w - 1, y + h - 1, color); // Right
}

// Draw a filled rectangle
void gfx_fill_rect(int x, int y, int w, int h, int color) {
    for (int py = y; py < y + h; py++) {
        for (int px = x; px < x + w; px++) {
            gfx_set_pixel(px, py, color);
        }
    }
}

// Draw a circle outline using midpoint algorithm
void gfx_circle(int cx, int cy, int r, int color) {
    int x = r;
    int y = 0;
    int err = 0;

    while (x >= y) {
        gfx_set_pixel(cx + x, cy + y, color);
        gfx_set_pixel(cx + y, cy + x, color);
        gfx_set_pixel(cx - y, cy + x, color);
        gfx_set_pixel(cx - x, cy + y, color);
        gfx_set_pixel(cx - x, cy - y, color);
        gfx_set_pixel(cx - y, cy - x, color);
        gfx_set_pixel(cx + y, cy - x, color);
        gfx_set_pixel(cx + x, cy - y, color);

        y++;
        if (err <= 0) {
            err += 2 * y + 1;
        }
        if (err > 0) {
            x--;
            err -= 2 * x + 1;
        }
    }
}

// Draw a filled circle
void gfx_fill_circle(int cx, int cy, int r, int color) {
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) {
                gfx_set_pixel(cx + x, cy + y, color);
            }
        }
    }
}

// Update the display (must be called to see changes)
void gfx_present(void) {
    @autoreleasepool {
        if (gView) {
            [gView setNeedsDisplay:YES];
            [gView displayIfNeeded];
        }
        // Process pending events
        NSEvent *event;
        while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                           untilDate:nil
                                              inMode:NSDefaultRunLoopMode
                                             dequeue:YES])) {
            [NSApp sendEvent:event];
        }
    }
}

// Check if window is still open
int gfx_running(void) {
    return gRunning ? 1 : 0;
}

// Wait for specified milliseconds (also processes audio)
void gfx_sleep(int ms) {
    // Use CFRunLoop to allow audio callbacks to fire during sleep
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, ms / 1000.0, false);
}

// Get screen width
int gfx_width(void) {
    return gWidth;
}

// Get screen height
int gfx_height(void) {
    return gHeight;
}

// Process pending events (call regularly to keep window responsive)
void gfx_poll_events(void) {
    @autoreleasepool {
        NSEvent *event;
        while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                           untilDate:nil
                                              inMode:NSDefaultRunLoopMode
                                             dequeue:YES])) {
            [NSApp sendEvent:event];
        }
    }
}

// Check if a key is available in buffer (non-blocking)
int gfx_key_pressed(void) {
    gfx_poll_events();
    return (gKeyHead != gKeyTail) ? 1 : 0;
}

// Read key non-blocking, returns -1 if no key available
// Arrow keys: Up=63232, Down=63233, Left=63234, Right=63235
int gfx_read_key(void) {
    gfx_poll_events();
    return popKey();
}

// Get the last key pressed (blocks until key press)
int gfx_get_key(void) {
    while (gRunning) {
        gfx_poll_events();
        int key = popKey();
        if (key >= 0) return key;
        usleep(10000);  // 10ms sleep to avoid busy wait
    }
    return 0;
}

// Get mouse X position
int gfx_mouse_x(void) {
    @autoreleasepool {
        if (gWindow) {
            NSPoint mouse = [gWindow mouseLocationOutsideOfEventStream];
            return (int)mouse.x;
        }
        return 0;
    }
}

// Get mouse Y position
int gfx_mouse_y(void) {
    @autoreleasepool {
        if (gWindow) {
            NSPoint mouse = [gWindow mouseLocationOutsideOfEventStream];
            return gHeight - (int)mouse.y;  // Flip Y coordinate
        }
        return 0;
    }
}

// Check if mouse button is pressed (button: 0=left, 1=right, 2=middle)
int gfx_mouse_button(int button) {
    @autoreleasepool {
        NSUInteger buttons = [NSEvent pressedMouseButtons];
        return (buttons & (1 << button)) ? 1 : 0;
    }
}

// ============================================================
// Sound Functions - Pascal callable
// ============================================================

// Initialize sound system (called automatically by snd_play)
void snd_init(void) {
    initAudio();
}

// Cleanup sound system
void snd_close(void) {
    cleanupAudio();
}

// Play a tone: frequency in Hz, duration in milliseconds
// waveform: 0=square, 1=sine, 2=noise
void snd_play(int frequency, int duration, int waveform) {
    initAudio();  // Ensure audio is initialized
    gSoundFrequency = frequency;
    gSoundDuration = (duration * SAMPLE_RATE) / 1000;
    gSoundPosition = 0;
    gSoundWaveform = waveform;
    gSoundPlaying = YES;

    // Give the audio system a moment to start
    // Process run loop briefly to let audio callback fire
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.001, false);
}

// Play a simple beep (square wave)
void snd_beep(int frequency, int duration) {
    snd_play(frequency, duration, 0);
}

// Play a tone (sine wave)
void snd_tone(int frequency, int duration) {
    snd_play(frequency, duration, 1);
}

// Play noise
void snd_noise(int duration) {
    snd_play(1000, duration, 2);
}

// Set volume (0-100)
void snd_volume(int vol) {
    if (vol < 0) vol = 0;
    if (vol > 100) vol = 100;
    gSoundVolume = vol;
}

// Check if sound is still playing
int snd_playing(void) {
    return gSoundPlaying ? 1 : 0;
}

// Wait for current sound to finish
void snd_wait(void) {
    while (gSoundPlaying) {
        usleep(1000);
    }
}
