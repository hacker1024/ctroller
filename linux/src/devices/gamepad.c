#include "devices.h"
#include "hid.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <linux/uinput.h>

static const struct uinput_user_dev gamepad = {
    .name = "Nintendo 3DS",
    .id =
        {
            .vendor  = 0x057e,
            .product = 0x0401,
            .version = 1,
            .bustype = BUS_VIRTUAL,
        },

    // Circlepad
    .absmin[ABS_X]  = -0x9c,
    .absmax[ABS_X]  = 0x9c,
    .absflat[ABS_X] = 10,
    .absfuzz[ABS_X] = 3,

    .absmin[ABS_Y]  = -0x9c,
    .absmax[ABS_Y]  = 0x9c,
    .absflat[ABS_Y] = 10,
    .absfuzz[ABS_Y] = 3,

    // C-Stick
    .absmin[ABS_RX]  = -0x9c,
    .absmax[ABS_RX]  = 0x9c,
    .absflat[ABS_RX] = 10,
    .absfuzz[ABS_RX] = 3,

    .absmin[ABS_RY]  = -0x9c,
    .absmax[ABS_RY]  = 0x9c,
    .absflat[ABS_RY] = 10,
    .absfuzz[ABS_RY] = 3,
    
    // Analogue DPAD
    .absmin[ABS_HAT0X] = -1,
    .absmax[ABS_HAT0X] = 1,
    .absflat[ABS_HAT0X] = 0,
    .absfuzz[ABS_HAT0X] = 0,
    
    .absmin[ABS_HAT0Y] = -1,
    .absmax[ABS_HAT0Y] = 1,
    .absflat[ABS_HAT0Y] = 0,
    .absfuzz[ABS_HAT0Y] = 0,
};

static uint16_t keys[] = {
    BTN_SOUTH,
    BTN_EAST,
    BTN_NORTH,
    BTN_WEST,
    
    BTN_START,
    BTN_SELECT,

    BTN_TL,
    BTN_TR,
    
    BTN_TL2,
    BTN_TR2,

    // We now send analogue dpad signals, instead of digital, due to a bug in android.
//    BTN_DPAD_UP,
//    BTN_DPAD_DOWN,
//    BTN_DPAD_LEFT,
//    BTN_DPAD_RIGHT,
};

static const uint32_t keymasks[] = {
    HID_KEY_A,
    HID_KEY_B,
    HID_KEY_X,
    HID_KEY_Y,

    HID_KEY_START,
    HID_KEY_SELECT,

    HID_KEY_L,
    HID_KEY_R,
    
    HID_KEY_ZL,
    HID_KEY_ZR,

    // We now send analogue dpad signals, instead of digital, due to a bug in android.
//    HID_KEY_DUP,
//    HID_KEY_DDOWN,
//    HID_KEY_DLEFT,
//    HID_KEY_DRIGHT,
};

static const uint16_t axis[] = {
    // Circlepad
    ABS_X,
    ABS_Y,

    // C-Stick
    ABS_RX,
    ABS_RY,
    
    // DPAD
    ABS_HAT0X,
    ABS_HAT0Y,
};

#define NUMEVENTS (arrsize(keys) + arrsize(axis) + 1)

struct device_context device_gamepad = {
    -1,
    gamepad_write,
    gamepad_create,
};

// This function loads the keymap into memory.
void load_keymap(const char *keymap_file_path) {
    printf("Loading keymap from file: %s\n", keymap_file_path);
    
    // Opens the specified keymap file and saves the reference in a pointer
    FILE *keymap_file = fopen(keymap_file_path, "r");
    
    // Here we make sure the keymap file actually exists.
    if(keymap_file == NULL) {
        fprintf(stderr, "Keymap file does not exist.\nReverting to default keymap.\n\n");
        return;
    }
    
    /* This bit of code counts the amount of lines in the keymap file, and makes sure there are 14.
     * It checks each character in turn, and increments "count" for every newline.
     * The loop breaks when it hits an EOF.
     * The stream postition indicator is then reset to the beginning of the file.
     */
    char c;
    int count = 0;
    while(1) {
        c = fgetc(keymap_file);
        if (c == '\n') count++;
        if( feof(keymap_file) ) { 
           break;
        }
    }
    rewind(keymap_file);
    if (count != arrsize(keys)) {
    fprintf(stderr, "Keymap file has an invalaid number of lines.\nPlease make sure you have mapped every key.\nReverting to default keymap.\n\n");
    return;
    }
    
    /* This is where the keymap is actally loaded from the file into an array.
     * Using a ton of if statements probably isn't the best way to do it, but
     * I don't know any other way.
     * 
     * The file gets read line by line, and if the line matches any button, that
     * button gets added to the keys array.
     * 
     * Have a look at the gamepad_write function to see why this works.
     */
    int i = 0;
    char buf[8];
    while (fgets(buf, sizeof(buf), keymap_file) != NULL) {
        buf[strlen(buf) - 1] = '\0'; // eat the newline fgets() stores
        
        printf("%hu -> \0", keys[i]);
        
        if (strcmp(buf, "A") == 0) keys[i] = BTN_SOUTH;
        if (strcmp(buf, "B") == 0) keys[i] = BTN_EAST;
        if (strcmp(buf, "X") == 0) keys[i] = BTN_NORTH;
        if (strcmp(buf, "Y") == 0) keys[i] = BTN_WEST;
        if (strcmp(buf, "START") == 0) keys[i] = BTN_START;
        if (strcmp(buf, "SELECT") == 0) keys[i] = BTN_SELECT;
        if (strcmp(buf, "ZL") == 0) keys[i] = BTN_TL2;
        if (strcmp(buf, "ZR") == 0) keys[i] = BTN_TR2;
        if (strcmp(buf, "L") == 0) keys[i] = BTN_TL;
        if (strcmp(buf, "R") == 0) keys[i] = BTN_TR;
        
        // We now send analogue dpad signals, instead of digital, due to a bug in android.
        // This makes it harder to map the dpad buttons, I'll have to think of a way to get around this.
//        if (strcmp(buf, "UP") == 0) keys[i] = BTN_DPAD_UP;
//        if (strcmp(buf, "DOWN") == 0) keys[i] = BTN_DPAD_DOWN;
//        if (strcmp(buf, "LEFT") == 0) keys[i] = BTN_DPAD_LEFT;
//        if (strcmp(buf, "DRIGHT") == 0) keys[i] = BTN_DPAD_RIGHT;
        
        printf("%hu\n", keys[i]);
        
        i++;
    }
    
    // Now the keymap file gets closed, we don't need it any more.
    fclose(keymap_file);
    printf("Keymap file loaded.\n");
}

int gamepad_create(const char *uinput_device)
{
    int uinputfd = device_open(uinput_device);
    if (uinputfd < 0) {
        goto failure_noclose;
    }

    int res;
    res = device_register_keys(uinputfd, keys, arrsize(keys));
    if (res != arrsize(keys)) {
        goto failure;
    }

    res = device_register_absaxis(uinputfd, axis, arrsize(axis));
    if (res != arrsize(axis)) {
        goto failure;
    }

    res = device_create(uinputfd, &gamepad);
    if (res < 0) {
        goto failure;
    }

    return uinputfd;

failure:
    close(uinputfd);
failure_noclose:
    fprintf(stderr, "Failed to initialize Gamepad.\n");
    return -1;
}

int gamepad_write(int uinputfd, struct hidinfo *hid)
{
    int res;
    static struct input_event events[NUMEVENTS];

    /* The uinput code written is in the same index as the 3DS event code recieved
    *  (the 3ds event codes are in the keymasks array, and the uinput ones in keys.)
    *  This is how the keymapping can be changed from a configuration file - the
    *  order of the keys array is changed accordingly.
    */
    size_t i = 0;
    for (; i < arrsize(keys); i++) {
        events[i].type = EV_KEY;
        events[i].code = keys[i];
        events[i].value =
            HID_HAS_KEY(hid->keys.held | hid->keys.down, keymasks[i]);
    }

    events[i].type  = EV_ABS;
    events[i].code  = ABS_X;
    events[i].value = hid->circlepad.dx;
    i++;

    events[i].type  = EV_ABS;
    events[i].code  = ABS_Y;
    events[i].value = -hid->circlepad.dy;
    i++;

    events[i].type  = EV_ABS;
    events[i].code  = ABS_RX;
    events[i].value = hid->cstick.dx;
    i++;

    events[i].type  = EV_ABS;
    events[i].code  = ABS_RY;
    events[i].value = -hid->cstick.dy;
    i++;
    
    events[i].type  = EV_ABS;
    events[i].code  = ABS_HAT0X;
    if (HID_HAS_KEY(hid->keys.held | hid->keys.down, HID_KEY_DLEFT)) {
        events[i].value = -1;
    } else {
        if (HID_HAS_KEY(hid->keys.held | hid->keys.down, HID_KEY_DRIGHT)) {
            events[i].value = 1;
        } else {
            events[i].value = 0;
        }
    }
    i++;
    
    events[i].type  = EV_ABS;
    events[i].code  = ABS_HAT0Y;
    if (HID_HAS_KEY(hid->keys.held | hid->keys.down, HID_KEY_DUP)) {
        events[i].value = -1;
    } else {
        if (HID_HAS_KEY(hid->keys.held | hid->keys.down, HID_KEY_DDOWN)) {
            events[i].value = 1;
        } else {
            events[i].value = 0;
        }
    }
    i++;

    events[i].type  = EV_SYN;
    events[i].code  = SYN_REPORT;
    events[i].value = 0;
    i++;

    res = write(uinputfd, events, i * sizeof(struct input_event));
    if (res < 0) {
        perror("Error writing key events");
    }
    return res;
}

