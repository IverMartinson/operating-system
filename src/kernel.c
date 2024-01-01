#define NUMBER_OF_COMMANDS 2

// List of available commands
char* listOfCommands[NUMBER_OF_COMMANDS] = {"help\0", "shutdown\0"};

// Flag to indicate whether the system should shut down
int shouldShutdown = 0;

// Define size_t as an alias for unsigned int
typedef unsigned int size_t;

// Pointer to the video memory
char *videoMemPtr = (char*)0xb8000;

// Current line and column positions
int ln = 0;
int cp = 10;

// Define uint16_t and uint8_t as aliases for unsigned short and unsigned char
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

// VGA ports and controller register indices
#define VGA_PORT_CTRL 0x3D4
#define VGA_PORT_DATA 0x3D5
#define VGA_REG_CURSOR_HIGH 0x0E
#define VGA_REG_CURSOR_LOW  0x0F

// Function to write a byte to a port
void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

// Function to set the cursor position on the screen
void setCursorPosition(uint16_t position) {
    outb(VGA_PORT_CTRL, VGA_REG_CURSOR_HIGH);
    outb(VGA_PORT_DATA, (position >> 8) & 0xFF);
    outb(VGA_PORT_CTRL, VGA_REG_CURSOR_LOW);
    outb(VGA_PORT_DATA, position & 0xFF);
}

// Function to reverse a string
void reverse(char* str, int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        ++start;
        --end;
    }
}

// Function to convert an integer to a string
char* itoa(int num, char* str, int base) {
    int i = 0;
    int isNegative = 0;

    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    if (num < 0 && base != 10) {
        isNegative = 1;
        num = -num;
    }

    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }

    if (isNegative && base == 10)
        str[i++] = '-';

    str[i] = '\0';
    reverse(str, i);

    return str;
}

// Function to print a single character to the screen
void printChar(char data) {
    if (data == '\n') {
        ln++;
        cp = 0;
    }

    videoMemPtr[160 * ln + cp * 2] = data;
    videoMemPtr[160 * ln + cp * 2 + 1] = 0x02;
    cp += 1;

    setCursorPosition(ln*80 + cp);

    return;
}

// Function to print a string to the screen
void print(char* data) {
    unsigned int j = 0;

    while (data[j] != '\0') {
        if (data[j] == '\n') {
            ln++;
            cp = -1;
        }
        else {
            videoMemPtr[160 * ln + cp * 2] = data[j];
            videoMemPtr[160 * ln + cp * 2 + 1] = 0x02;
        }

        ++j;
        cp += 1;
    }

    setCursorPosition(ln*80 + cp);

    return;
}

// Function to print an integer to the screen
void printInt(int data) {
    char buffer[20];
    itoa(data, buffer, 10);
    print(buffer);
}

// Function to clear the screen
void clearScreen() {
    unsigned int i = 0;

    for (i = 0; i < 80 * 25 * 2; i += 2) {
        videoMemPtr[i] = ' ';
        videoMemPtr[i + 1] = 0x02;
    }

    return;
}

// Function to read a key from the keyboard
char readKey() {
    char keyBuffer;

    asm volatile (
        "waitForKey:\n"
        "inb $0x64, %0\n"
        "testb $0x01, %0\n"
        "jz waitForKey\n"
        "inb $0x60, %0\n"
        : "=a" (keyBuffer)
        : 
        : 
    );

    if (keyBuffer >= 2 && keyBuffer <= 57) {
        char keyMap[] = {
            '\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
            '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
            '\n', '\0', 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
            '\0', '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', '\0', '\0', '\0', ' '
        };

        return keyMap[keyBuffer];
    }

    return '\0';
}

// Function to compare two strings
int cstrcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

// Function to copy a string with specified length
char* cstrncpy(char* dest, const char* src, size_t n) {
    char* original_dest = dest;

    while (n > 0 && *src != '\0') {
        *dest = *src;
        dest++;
        src++;
        n--;
    }

    while (n > 0) {
        *dest = '\0';
        dest++;
        n--;
    }

    return original_dest;
}

// Function to check and execute commands
void checkCommand(char command[10][21]){
    if (cstrcmp(command[0], "help") == 0){
        for (int j = 0; j < NUMBER_OF_COMMANDS; j++) {
            print(listOfCommands[j]);
            if (j < NUMBER_OF_COMMANDS - 1) print("\n ");
        }
    }
    else if (cstrcmp(command[0], "shutdown") == 0){
        shouldShutdown = 1;
        print("[INFO] Shutting down...");
    } else{
        print("[ERROR] Command not found. Try 'help' for a list of commands");
    }
    return;
}

// Main kernel function
void kernelMain() {
    clearScreen();
    print("Welcome to MegaDumbOS: the Operating System of the Future");
    ln += 3;
    cp = 0;

    char currentToken[21] = {'\0'};
    char command[10][21] = {};
    int token = -1;
    int canDelete = 0;

    // Command prompt
    print("=> ");

    while (1) {
        char key = readKey();

        if (key == '\n'){
            token++;
            
            cstrncpy(command[token], currentToken, 20);
            command[token][20] = '\0';
            currentToken[0] = '\0';

            print("\n ");

            checkCommand(command);

            if (shouldShutdown == 1) return;

            for (int i = 0; i <= token; i++) {
                command[i][0] = '\0';
            }

            token = -1;
            canDelete = 0;
            print("\n=> ");
        }

        else if (key == '\b'){
            if (canDelete <= 0) continue;
            
            int i = 0;
            while (currentToken[i] != '\0') {
                i++;
            }

            currentToken[i - 1] = '\0';

            cp--;

            printChar('\0');
        
            cp--;
            setCursorPosition(ln*80 + cp);

            canDelete--;
        }

        else if (key != '\0'){
            int i = 0;
            while (currentToken[i] != '\0') {
                i++;
            }

            currentToken[i] = key;
            currentToken[i + 1] = '\0';

            canDelete++;

            printChar(key);
        }

        if (key == ' '){
            token++;
            
            cstrncpy(command[token], currentToken, 20);
            command[token][20] = '\0';

            currentToken[0] = '\0';
        }
    }

    return;
}
