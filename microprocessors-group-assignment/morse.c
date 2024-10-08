#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <time.h>
#include "ws2812.pio.h"
#include "hardware/watchdog.h"


#define IS_RGBW true        //set rgwb true
#define NUM_PIXELS 150      //set pixels,  copied from pico sdk examples tutorial
#define WS2812_PIN 28       // set rgb pin 28, copied from pico sdk examples

#define SEQ_TOTAL 5         // sequence is 5 in length with 2 spaces 5 - 2 = 3 characters or words
#define NUM_STAGES_PER_LEVEL 5  //stages

//statistics
int liveslost;
int livesgained;
int sequencesgenerated;
double stagesTotal;
double stagesCorrect;
double stagePercent;

// index and array definitions
int bufferLen = 100;            
int morseindex = 0;
char *sequence_array[5];
int morse_sequence_array[5];
char morseString[100];
char expected_morse[100];
char morse_usrinput[100];
int morse_array[100];
char arraytostring[100];


static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

// pixel pico sdk tutorial rgb instantiation
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return
            ((uint32_t) (r) << 8) |
            ((uint32_t) (g) << 16) |
            (uint32_t) (b);
}



bool printhelp = true;  //for printing morse underneath

char dot = '.';
char dash = '-';
char space = ' ';
char null = '\n';
int time1;          //rising edge
int time2;          //falling
int timediff;       //difference
int level;      
int lives;
const int level_difficulty[] = {1, 2, 3, 4};

//patterns stored as integers to convert
const int level_patterns[] = {
    1222,                         // -> Level 1 (EASY)
    1122,                         // -> Level 2 (MEDIUM)
    1112,                         // -> Level 3 (HARD)
    1111                         // -> Level 4 (MILITARY GRADE)
};


// Defines Morse code representations from A-Z
const int morse_alphabet[] = {
    12,          //".-",   // A
    2111,        //"-...", // B
    2121,        //"-.-.", // C
    211,         //"-..",  // D
    1,           //".",    // E
    1121,        //"..-.", // F
    221,         //"--.",  // G
    1111,        //"....", // H
    11,          //"..",   // I
    1222,        //".---", // J
    212,         //"-.-",  // K
    1211,        //".-..", // L
    22,          //"--",   // M
    21,          //"-.",   // N
    222,         //"---",  // O
    1221,        //".--.", // P
    2212,        //"--.-", // Q
    121,         //".-.",  // R
    111,         //"...",  // S
    2,           //"-",    // T
    112,         //"..-",  // U
    1112,        //"...-", // V
    122,         //".--",  // W
    2112,        //"-..-", // X
    2122,        //"-.--", // Y
    2211,         //"--.." // Z
    12222,       //".----" // 1   
    11222,       //"..---" // 2 
    11122,       //"...--" // 3 
    11112,       //"....-" // 4 
    11111,       //"....." // 5 
    21111,       //"-...." // 6
    22111,       //"-...." // 7   
    22211,       //"-...." // 8    
    22221,       //"-...." // 9
    22222        //"-...." // 0

};

const char* helpalphabet[] = {
    ".-",       // A
    "-...",     // B
    "-.-.",     // C
    "-..",      // D
    ".",        // E
    "..-.",     // F
    "--.",      // G
    "....",     // H
    "..",       // I
    ".---",     // J
    "-.-",      // K
    ".-..",     // L
    "--",       // M
    "-.",       // N
    "---",      // O
    ".--.",     // P
    "--.-",     // Q
    ".-.",      // R
    "...",      // S
    "-",        // T
    "..-",      // U
    "...-",     // V
    ".--",      // W
    "-..-",     // X
    "-.--",     // Y
    "--..",     // Z
    ".----",    // 1
    "..---",    // 2
    "...--",    // 3
    "....-",    // 4
    ".....",    // 5
    "-....",    // 6
    "--...",    // 7
    "---..",    // 8
    "----.",    // 9
    "-----"     // 0
};



const char alphabet[] = {
    'A',          //".-",   // A
    'B',        //"-...", // B
    'C',        //"-.-.", // C
    'D',         //"-..",  // D
    'E',           //".",    // E
    'F',        //"..-.", // F
    'G',         //"--.",  // G
    'H',        //"....", // H
    'I',          //"..",   // I
    'J',        //".---", // J
    'K',         //"-.-",  // K
    'L',        //".-..", // L
    'M',          //"--",   // M
    'N',          //"-.",   // N
    'O',         //"---",  // O
    'P',        //".--.", // P
    'Q',        //"--.-", // Q
    'R',         //".-.",  // R
    'S',         //"...",  // S
    'T',           //"-",    // T
    'U',         //"..-",  // U
    'V',        //"...-", // V
    'W',         //".--",  // W
    'X',        //"-..-", // X
    'Y',        //"-.--", // Y
    'Z',         //"--.."  // Z
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    '0'
    };

char *words[] = {
    "Dance",
    "Smile",
    "River",
    "Cloud",
    "Beach",
    "Earth",
    "Music",
    "Storm",
    "Happy",
    "Dream",
    "Space",
    "Light",
    "Flower",
    "Ocean",
    "Meadow",
    "Birds",
    "Quiet",
    "Grace",
    "Shine",
    "Bliss",
    "Peace",
    "Heart",
    "Seren",
    "Magic",
    "Amber",
    "Radii",
    "Frost",
    "Breeze",
    "Dream",
    "Dawn",
    "Faith",
    "Flame",
    "Meadow",
    "Poise",
    "Swirl",
    "Zesty"
};

char *wordshelp[] = {
    "-.. .- -. -.-. .",   // Dance
    "... -- .. .-.. .",   // Smile
    ".-. .. ...- .",      // River
    "-.-. .-.. --- ..- -..",  // Cloud
    "-... . .- -.-.",     // Beach
    ". .- .-. - ....",    // Earth
    "-- ..- ... .. -.-.", // Music
    "... - --- .-. --",   // Storm
    ".... .- .--. .--. -.--",  // Happy
    "-.. .-. . .- --",    // Dream
    "... .--. .- -.-. .", // Space
    ".-.. .. --. .... -", // Light
    "..-. .-.. --- .-- . .-.",  // Flower
    "--- -.-. .- -.",     // Ocean
    "-- . .- -.. --- .--",// Meadow
    "-... .. .-. -.. ...",// Birds
    "--.- ..- .. . -",    // Quiet
    "--. .-. .- -.-.",    // Grace
    "... .... .. -. .",   // Shine
    "-... .-.. .. ... ...",    // Bliss
    ".--. . .- -.-.",     // Peace
    ".... . .- .-. -",    // Heart
    "... . .-. . -. -",   // Seren
    "-- .- --. .. -.-.",  // Magic
    ".- -- -... . .-.",   // Amber
    ".-. .- -.. .. ..",   // Radii
    "..-. .-. --- ... -", // Frost
    "-... .-. . . --.. .",// Breeze
    "-.. .-. . .- --",    // Dream
    "-.. .- .-- -.",      // Dawn
    "..-. .- .. - ....",  // Faith
    "..-. .-.. .- -- .",  // Flame
    "-- . .- -.. --- .--",// Meadow
    ".--. --- .. ... .",  // Poise
    "... .-- .. .-. .-..",// Swirl
    "--.. . ... - -.--"   // Zesty
};



// Declare the main assembly code entry point.
void main_asm();

// Initialise a GPIO pin – see SDK for detail on gpio_init()
void asm_gpio_init(uint pin) {
    gpio_init(pin);
}

// Set direction of a GPIO pin – see SDK for detail on gpio_set_dir()
void asm_gpio_set_dir(uint pin, bool out) {
    gpio_set_dir(pin, out);
}

// Get the value of a GPIO pin – see SDK for detail on gpio_get()
bool asm_gpio_get(uint pin) {
    return gpio_get(pin);
}

// Set the value of a GPIO pin – see SDK for detail on gpio_put()
void asm_gpio_put(uint pin, bool value) {
    gpio_put(pin, value);
}

 // need to set the interrupts to fire when the button is pressed and let go, so OR them
// Enable falling-edge interrupt – see SDK for detail on gpio_set_irq_enabled()
void asm_gpio_set_irq_fall(uint pin) {
    gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_FALL, true); 
}

void asm_gpio_set_irq_rise(uint pin) {
    gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_RISE, true); 
}

void time_check1() {
    time1 = time_us_64();
}

void time_check2() {
    time2 = time_us_64();
}

int time_diff() {
    timediff = time2 - time1;
    return timediff;
}

void print_welcome_screen(){
printf("\033[0;34m"); 
    // Print the ASCII art welcome message
    printf("           /\\                                                                                                            /\\\n");
    printf("          /  \\                                                                                                          /  \\   \n");
    printf("         /    \\                                                                                                        /    \\  \n");
    printf("         \\     \\                                                                                                      /     /     \n");
    printf("          \\     \\                                                                                                    /     /      \n");
    printf("          /   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~      \\     \n");                                                     
    printf("         /   ||  ███╗   ███╗ ██████╗ ██████╗ ███████╗███████╗    ███╗   ███╗ █████╗ ███████╗████████╗███████╗██████╗   ||    \\  \n");
    printf("        /    ||  ████╗ ████║██╔═══██╗██╔══██╗██╔════╝██╔════╝    ████╗ ████║██╔══██╗██╔════╝╚══██╔══╝██╔════╝██╔══██╗  ||     \\  \n");
    printf("       /     ||  ██╔████╔██║██║   ██║██████╔╝███████╗█████╗      ██╔████╔██║███████║███████╗   ██║   █████╗  ██████╔╝  ||      \\ \n");
    printf("       \\     ||  ██║╚██╔╝██║██║   ██║██╔══██╗╚════██║██╔══╝      ██║╚██╔╝██║██╔══██║╚════██║   ██║   ██╔══╝  ██╔══██╗  ||      /    \n");
    printf("        \\    ||  ██║ ╚═╝ ██║╚██████╔╝██║  ██║███████║███████╗    ██║ ╚═╝ ██║██║  ██║███████║   ██║   ███████╗██║  ██║  ||     /   \n");
    printf("         \\   ||  ╚═╝     ╚═╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝╚══════╝    ╚═╝     ╚═╝╚═╝  ╚═╝╚══════╝   ╚═╝   ╚══════╝╚═╝  ╚═╝  ||    /   \n");
    printf("         /     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~      \\   \n");
    printf("        /       /                                                                                                    \\       \\ \n");
    printf("        \\      /                                                                                                      \\      /  \n");
    printf("         \\    /                                                                                                        \\    /       \n");
    printf("          \\  /                                                                                                          \\  /     \n");
    printf("           \\/                                                                                                            \\/      \n");
    printf ("                                                *********************************************                  \n");
    printf ("                                                 *                                           *                  \n");
    printf ("                                                 *         Welcome to Morse Master!          *                  \n");
    printf ("                                                 *  A Morse Code Game developed by Group 12  *                  \n");
    printf ("                                                 *       Learn Morse Code the Fun Way!       *                  \n");
    printf ("                                                 *                                           *                  \n");
    printf ("                                                 *********************************************                  \n");
    printf ("\n");
    printf ("\n");
    printf ("\n");
    printf ("\n");
    printf ("\n");
    printf ("\n");
    printf(" HOW TO PLAY                                                                ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⣤⣴⣶⣶⣦⣤⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣤⣦⣶⣶⣤⣤⣄⡀⠀⠀⠀⠀  \n");
    printf("* You will be shown a letter or a number                                    ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣸⡿⠾⠛⣛⠛⠻⢿⣯⣤⣤⣤⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣀⣤⣤⣤⣿⡿⠟⣛⣛⣛⢳⢿⣇⠀⠀\n");
    printf("* Enter the morse code equivalent of the character shown                    ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⡞⣣⠔⠋⢉⣉⣉⠉⠳⢌⠳⣍⠉⠉⠉⠭⠭⠭⠭⠭⠭⠭⠍⠉⠉⠉⠉⠉⠉⣩⢞⡵⠚⢉⣷⣶⣄⠉⠒⢬⡳⡄⠀⠀⠀⠀\n");
    printf("* To do this, press the button quickly for a dot or press longer for a dash ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⡏⡖⠁⠀⠀⣿⣿⣿⡆⠀⠀⢣⠙⡆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣰⢳⠚⠀⠀⠸⣿⣿⡿⠀⠀⠀⢣⢻⡆⠀⠀⠀\n");
    printf("* After each dot/dash, wait a second before putting in the next dot/dash    ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣾⢡⠀⢰⣾⣶⣿⣿⣿⣷⣿⣷⠀⡆⣿⠀⢀⣤⣤⡀⠀⠀⠀⠀⠀⢀⣤⣤⡀⠀⡏⣠⣾⣿⣷⡀⠀⠀⠀⣰⣿⣿⣦⡆⣿⠀⠀⠀\n");
    printf("* After each letter wait 3 seconds to show that you're done                 ⠀⠀⠀⠀⠀⠀⠀⠀⠀⢰⣿⡇⠄⠸⠿⠿⣿⣿⣿⡿⠿⠟⠀⠃⡟⠀⠘⠛⠛⠃⠀⠀⠀⠀⠀⠘⠛⠛⠃⠀⢷⠈⠻⠿⠟⠁⣀⣀⡀⠘⠿⠿⠏⢣⣿⡇⠀\n");
    printf("* All players start with 3 lives                                            ⠀⠀⠀⠀⠀⠀⠀⠀⠀⣸⡷⠹⣄⠀⠀⠀⣿⣿⣿⠇⠀⣠⠖⢋⣋⣉⡙⠲⣄⠀⠀⠀⠀⠀⠀⠀⣠⠖⢋⣉⣉⡛⠲⣄⠀⢰⣿⣿⣿⠀⠀⠀⢀⡞⢼⣷⠀⠀\n");
    printf("* If you enter the letter or number correctly, you get a life :)            ⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⠃⠀⠙⠳⣄⡀⠀⠉⠁⢀⡼⢁⣶⣿⣿⣿⣿⣦⡈⢧⠀⠀⠀⠀⠀⡼⢁⣾⣿⣿⣿⣿⣦⡈⣧⡈⠿⠿⠯⢀⣠⠴⠋⠀⠈⣿⡀⠀\n");
    printf("* If not, you lose a life :(                                                ⠀⠀⠀⠀⠀⠀⠀⠀⢰⣿⠀⠀⠀⠀⠀⠉⠉⠛⠉⠉⡇⢸⣿⣿⣿⣿⣿⣿⡇⢸⠀⠀⠀⠀⠐⡇⢸⣿⣿⣿⣿⣿⣿⡇⢸⠉⠙⠛⠉⠉⠀⠀⠀⠀⠀⢿⡇⠀\n");
    printf("* If you get 5 right in a row, you level up                                 ⠀⠀⠀⠀⠀⠀⠀⠀⢸⡏⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢳⡈⢿⣿⣿⣿⣿⡿⢡⣞⣀⣀⣀⣀⣀⣳⡘⢿⣿⣿⣿⣿⡿⢡⡞⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣧⠀\n");
    printf("* If you lose all your lives, then its GAME OVER                            ⠀⠀⠀⠀⠀⠀⠀⠀⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⣿⣦⣌⣉⣉⣉⣴⡿⠛⠛⠛⠛⠛⠛⠛⢿⣦⣌⣉⣉⣉⣴⣿⡆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠘⣿⠀\n");
    printf("* Have fun playing and may the force be with you.....                       ⠀⠀⠀⠀⠀⠀⠀⠀⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣾⠇⠈⠉⠛⠛⠋⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠙⠛⠛⠋⠁⠘⣿⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⡀\n");
    printf("                                                                            ⠀⠀⠀⠀⠀⠀⠀⠀⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣾⠏⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠹⣷⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⡇\n");                                       
    printf("                                                                            ⠀⠀⠀⠀⠀⠀⠀⠀⣿⡄⠀⠀⠀⠀⠀⠀⠀⢀⣼⠏⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠹⣷⡀⠀⠀⠀⠀⠀⠀⠀⢠⣿⠀\n");
    printf("                                                                            ⠀⠀⠀⠀⠀⠀⠀⠀⠘⢿⣄⠀⠀⠀⠀⠀⢀⣾⠏⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⣷⡄⠀⠀⠀⠀⠀⣠⣿⠃⠀\n");
    printf("                                                                            ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠙⣿⣶⣤⣤⣶⡟⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢻⣶⣤⣤⣶⣿⠛⠁⠀\n");
    printf("                                                                            ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠉⠉⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠉⠉⠉⠀⠀⠀⠀⠀\n");
    printf("\033[0m");
}

//checks output from arm and compares to place in array, which later gets converted to a string
void morse_parse(int morse_value) {


    if (morse_value == 1) {                     //if 1 (dot) insert 1 into index at 0
        morse_array[morseindex] = 1;
        printf("%c", dot);  
        //printf("%d", morse_value);
        morseindex++;                               //increment index 
    }
    if (morse_value == 2) {
        morse_array[morseindex] = 2;
        printf("%c", dash);
        // printf("%d", morse_value);
        morseindex++;
    }
    if (morse_value == 3) {
        if (morseindex != 0) {                      //stops printing space at beginning of new input
            morse_array[morseindex] = 3;
            printf("%c", space);
            // printf("%d", morse_value);
            morseindex++;
        }
    }
    if (morse_value == 4) {                 
        if (morseindex != 0) {                   //stops printing finish at beginning of new input
            morseindex = bufferLen;             //sets index as buffer length to end input
            printf("%c", null);

        }    
    }
}

/**
 * @brief Creates an integer from an array of integers.
 * 
 * This function takes an array of integers representing digits and creates
 * an integer from them. The array is assumed to contain digits in the range
 * [0, 9]. Each element of the array contributes to the resulting integer's
 * magnitude according to its position in the array.
 * 
 * @param morse_array The array of integers containing digits.
 * @param length The length of the array.
 * @return The integer created from the array.
 */
int create_int(int morse_array[], int length) {
    int k = 0;  /**< Resulting integer */
    //printf("\n");
    for (int i = 0; i < length; i++) {                  
        k = 10 * k + morse_array[i]; /**< Constructing the integer */
    }
    return k;   /**< Return the resulting integer */
}

/**
 * @brief Reads input from the morse_array and converts it into an integer.
 * 
 * This function reads input from the morse_array and converts it into
 * an integer. It fills the array with terminators to wipe it before
 * getting new input. The function then constructs an integer from the
 * array using the create_int function and returns it.
 * 
 * @param bufferLen The length of the buffer array.
 * @return The integer created from the morse_array.
 */
int input(int bufferLen) {
    // Fill the array with terminators to wipe it before getting new input
    for (int n = 0; n < 100; n++) {
        morse_array[n] = '\0';
    }
    
    // Continuously check if the morseindex has reached the bufferLen
    while (1) {
        if (morseindex == bufferLen) {
            morseindex = 0; // Reset the array
            break;
        }
    }
  
    // Create an integer from the morse_array
    int number = create_int(morse_array, bufferLen);
    
    // Return the resulting integer
    return number;
}


/**
 * @brief Converts the expected_morse sequence into a numerical representation.
 * 
 * This function converts the expected_morse sequence into a numerical representation
 * stored in the morseString array. Each dot ('.'), dash ('-'), and space (' ') in the
 * expected_morse sequence is converted into the corresponding numerical value and stored
 * in the morseString array.
 * 
 * @note The expected_morse sequence must be null-terminated for proper processing.
 */
void sequenceMorse() {
    bufferLen = (strlen(expected_morse)); // Sets buffer length based on length of generated sequence
    // printf("Buffer length: %d", bufferLen);

    // Convert each character in the expected_morse sequence into a numerical representation
    for (int i = 0; i < strlen(expected_morse); i++) {
        if (expected_morse[i] == '.') {
            morseString[i] = '1'; // Dot representation
        }
        if (expected_morse[i] == '-') {
            morseString[i] = '2'; // Dash representation
        }
        if (expected_morse[i] == ' ') {
            morseString[i] = '3'; // Space representation
        }
    }

    // Fill the expected_morse, morseString, and arraytostring arrays with terminators to wipe them
    for (int n = 0; n < 100; n++) {
        expected_morse[n] = '\0';
        // morseString[n] = '\0'; // Uncomment if morseString is used elsewhere
        arraytostring[n] = '\0';
    }
    // printf("\nwiped expected morse");
}

/**
 * @brief Generates a sequence of random words and their Morse code representations.
 * 
 * This function generates a sequence of random words based on the specified level.
 * The words are randomly selected from a predefined list of words. If the level is
 * not the highest (level 4), the function also prints the Morse code representations
 * of the words to provide assistance. The generated sequence is stored in the 
 * sequence_array and expected_morse arrays.
 * 
 * @param level The level of difficulty for the word sequence generation.
 *              Values: 1, 2, 3, 4 (higher levels provide less assistance).
 */
void randomWords(int level) {
    srand(time(NULL));  // Seed the random number generator with the current time
    
    int totalwords = 30;  // Total number of words in the predefined list
    int length = 0;  // Length of the expected_morse sequence
    bool help = true;  // Flag to determine whether to print Morse code representations
    
    // Disable assistance if the level is the highest (level 4)
    if (level == 4) {
        help = false;
    }

    printf("\nMake this Sequence!:\n");

    // Generate the word sequence and their Morse code representations
    for (int i = 0; i < SEQ_TOTAL; i += 2) {
        int index = rand() % totalwords;  // Generate a random index to select a word
        sequence_array[i] = words[index];  // Store the word in the sequence_array
        sequence_array[i + 1] = " ";  // Store a space to separate words
        
        // Print the Morse code representation if assistance is enabled
        if (help) {
            printf("%s ", wordshelp[index]);
        }

        // Concatenate the Morse code representation to the expected_morse sequence
        strcat(expected_morse, wordshelp[index]);
        length = strlen(expected_morse);
        expected_morse[length] = ' ';  // Add a space to separate Morse code representations
    }
    printf("\n");
    
    // Print the generated word sequence
    for (int n = 0; n < SEQ_TOTAL; n++) {
        printf("    %s   ", sequence_array[n]);
    }
    printf("\n");

    // Convert the expected_morse sequence into numerical representation
    sequenceMorse();

    sequencesgenerated++;  // Increment the count of generated sequences
}


/**
 * @brief Generates a random sequence of characters and their Morse code representations.
 * 
 * This function generates a random sequence of characters based on the specified level.
 * The characters are randomly selected from a predefined list of alphanumeric characters.
 * If the level is not the second level, the function also prints the Morse code representations
 * of the characters to provide assistance. The generated sequence is stored in the 
 * sequence_array and expected_morse arrays.
 * 
 * @param level The level of difficulty for the character sequence generation.
 *              Values: 1, 2 (higher levels provide less assistance).
 */
void randomSequence(int level) {
    srand(time(NULL));  // Seed the random number generator with the current time
    
    char characters[36] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";  // Predefined list of characters
    int totalCharacters = 36;  // Total number of characters in the predefined list
    int length = 0;  // Length of the expected_morse sequence
    bool help = true;  // Flag to determine whether to print Morse code representations
    
    // Disable assistance if the level is the second level
    if (level == 2) {
        help = false;
    }
    
    printf("\nMake this Sequence!:\n");

    // Generate the character sequence and their Morse code representations
    for (int i = 0; i < SEQ_TOTAL; i += 2) {
        int index = rand() % totalCharacters;  // Generate a random index to select a character
        sequence_array[i] = characters[index];  // Store the character in the sequence_array
        sequence_array[i + 1] = ' ';  // Store a space to separate characters
        
        // Print the Morse code representation if assistance is enabled
        if (help) {
            printf("%s ", helpalphabet[index]);
        }

        // Concatenate the Morse code representation to the expected_morse sequence
        strcat(expected_morse, helpalphabet[index]);
        length = strlen(expected_morse);
        expected_morse[length] = ' ';  // Add a space to separate Morse code representations
    }
    printf("\n");

    // Print the generated character sequence
    for (int n = 0; n < SEQ_TOTAL; n++) {
        printf(" %c ", sequence_array[n]);
    }
    printf("\n");

    // Convert the expected_morse sequence into numerical representation
    sequenceMorse(); // Makes the random sequence into a string for comparison
    sequencesgenerated++;  // Increment the count of generated sequences
}





/**
 * @brief Prompts the user to select a difficulty level and returns the chosen level.
 * 
 * This function displays a menu of available difficulty levels along with their Morse code
 * representations. It prompts the user to input the Morse code corresponding to their chosen
 * difficulty level. The function then compares the input with predefined patterns and returns
 * the difficulty level selected by the user.
 * 
 * @return The chosen difficulty level.
 */
int level_picker() {
    printf("Please select a level!\n\n"
           "Level 1 - .--- \n"
           "Level 2 - ..-- \n"
           "Level 3 - ...- \n"
           "Level 4 - .... \n");
    bufferLen = 4;  // Set the buffer length for input to 4 characters
    int number = input(bufferLen);  // Get input from the user and convert it into an integer

    // Compare the input with the predefined patterns for each difficulty level
    for (int i = 0; i < 4; ++i) {
        if (number == level_patterns[i]) {
            printf("\nLevel %d selected!\n", level_difficulty[i]);
            return level_difficulty[i];  // Return the corresponding difficulty level
        }
    }

    // If the input does not match any predefined pattern, select level 1 as the default
    printf("\nInvalid input! Selecting level 1.\n");
    return 1;  // Return level 1 as the default choice
}



/**
 * @brief Prompts the user to input Morse code and converts it into numerical representation.
 * 
 * This function prompts the user to input Morse code. It reads the input from the morse_array
 * and converts it into a numerical representation stored in the arraytostring array. The function
 * then proceeds to process the Morse code input.
 */
void get_user_input() {
    printf("\nUser input begin! \n");
    int number = input(bufferLen);  // Get Morse code input from the morse.S file

    int count = 0;  // Initialize a counter to keep track of the number of elements processed
    // Convert each element of morse_array into its numerical representation
    for (int i = 0; i < bufferLen; i++) {
        if (morse_array[i] == 1) {
            arraytostring[i] = '1';  // Representing a dot
        }
        if (morse_array[i] == 2) {
            arraytostring[i] = '2';  // Representing a dash
        }
        if (morse_array[i] == 3) {
            arraytostring[i] = '3';  // Representing a space
        }
        count++;  // Increment the counter
    }
}

/**
 * @brief Wipes user input arrays.
 * 
 * This function wipes the user input arrays, namely morseString and morse_array,
 * by filling them with null terminators ('\0'). This is typically done to clear
 * the arrays before obtaining new input.
 */
void wipeUserInput() {
    // Fill the morseString and morse_array arrays with null terminators to wipe them
    for (int n = 0; n < 100; n++ ){
        morseString[n] = '\0';  // Wipe with null terminators
        morse_array[n] = '\0';
    }
}


/**
 * @brief Prints the current level and stage.
 * 
 * This function prints the current level and stage to the console.
 * 
 * @param level The current level of the game.
 * @param stage The current stage within the level.
 */
void print_level(int level, int stage) {
    printf("Level: %d, Stage %d\n", level, stage);
}

/**
 * @brief Updates the RGB LED color based on the number of lives left.
 * 
 * This function updates the color of the RGB LED based on the current number
 * of lives left in the game. It uses a switch statement to determine the color
 * based on the number of lives remaining.
 * 
 * @note This function assumes the presence of a function put_pixel(uint32_t pixel_grb)
 *       that updates the RGB LED with the specified color.
 */
void update_rgb_colour() {
    // TODO: Update the RGB based on number of lives left
    switch (lives) {
        case 3: // green: game has started / 3 lives remaining
            put_pixel(urgb_u32(0,255,0));
            break;
        case 2: // olive-green (128, 128, 0): 2 lives remaining
            put_pixel(urgb_u32(128,128,0));
            break;
        case 1: // reddish-orange (255, 128, 0): 1 life remaining
            put_pixel(urgb_u32(255,128,0));
            break;
        case 0: // red: game over / 0 lives remaining
            put_pixel(urgb_u32(255,0,0));
            break;
        default:
            break;
    }
}

/**
 * @brief Prints the player wins screen.
 * 
 * This function prints a screen indicating that the player has won the game,
 * along with some statistics such as lives lost, lives gained, and sequences generated.
 */
void print_player_wins_screen() {
    printf("\n\n");
    printf("██╗    ██╗ ██████╗ ╗███╗   ██╗\n");
    printf("██║    ██║██╔═══██╗║████╗  ██║\n");
    printf("██║ █╗ ██║██║   ██║║██╔██╗ ██║\n");
    printf("██║███╗██║██║   ██║║██║╚██╗██║\n");
    printf("╚███╔███╔╝╚██████╔╝║██║ ╚████║ \n");
    printf(" ╚══╝╚══╝  ╚═════╝   ╚═╝ ╚═══╝ \n");
    printf("\n");
    printf("Congratulations! You've won the game!\n\n");
    printf("Statistics:\n");
    printf("Lives Lost: %d\n", liveslost);
    printf("Lives Gained: %d\n", livesgained);
    printf("Sequences Generated: %d\n", sequencesgenerated);
}


/**
 * @brief Prints the game over screen.
 * 
 * This function prints a screen indicating that the game is over, along with some statistics
 * such as lives lost, lives gained, and sequences generated.
 */
void print_game_over_screen() {
    printf(" ██████╗  █████╗ ███╗   ███╗███████╗     ██████╗ ██╗   ██╗███████╗██████╗ \n");
    printf("██╔════╝ ██╔══██╗████╗ ████║██╔════╝    ██╔═══██╗██║   ██║██╔════╝██╔══██╗\n");
    printf("██║  ███╗███████║██╔████╔██║█████╗      ██║   ██║██║   ██║█████╗  ██████╔╝\n");
    printf("██║   ██║██╔══██║██║╚██╔╝██║██╔══╝      ██║   ██║╚██╗ ██╔╝██╔══╝  ██╔══██╗\n");
    printf("╚██████╔╝██║  ██║██║ ╚═╝ ██║███████╗    ╚██████╔╝ ╚████╔╝ ███████╗██║  ██║\n");
    printf(" ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝     ╚═════╝   ╚═══╝  ╚══════╝╚═╝  ╚═╝\n");
    printf("Game Over. Try again next time!\n");
    printf("Statistics:\n");
    printf("Lives Lost: %d\n", liveslost);
    printf("Lives Gained: %d\n", livesgained);
    printf("Sequences Generated: %d\n", sequencesgenerated);
}



/**
 * @brief Main entry point of the application.
 * 
 * This function serves as the main entry point of the application. It initializes
 * necessary components, sets up the game, and handles game logic until the game
 * ends either by the player winning or losing.
 * 
 * @return An integer representing the application return code.
 */
int main() {
    stdio_init_all();              // Initialise all basic IO

    watchdog_enable(8200, 1);

    /*
    RGB colours meaning:
        blue: game not in progress
        green: game has started / 3 lives remaining
        olive-green (128, 128, 0): 2 lives remaining
        reddish-orange (255, 128, 0): 1 life remaining
        red: game over / 0 lives remaining
    */

    PIO pio = pio0;                                                                                                                                                                                                
    int sm = 0;                                                                                                                                                                                                    
    uint offset = pio_add_program(pio, &ws2812_program);                                                                                                                                                           
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);
    
    put_pixel(urgb_u32(0,0,255));

    print_welcome_screen();
    main_asm();

    
    lives = 3;
    level = level_picker();

    
    
    update_rgb_colour();
    while (lives > 0){
        // TODO: Set RGB to green to indicate game has started

        if (level < 5) {    // Level 5 indicates game over, player wins
            bool proceed_to_next_level = true;
            for (int stage = 0; stage < NUM_STAGES_PER_LEVEL; stage++)
            {
                stagesTotal++;

                if (level < 3){     // level is 1 or 2
                    print_level(level, stage);
                    randomSequence(level);
                    get_user_input(); 
                    
                }
                else if (level > 2){   // level is 3 or 4
                    print_level(level, stage);
                    randomWords(level);
                    get_user_input();  
                }
 
                if (strcmp(morseString, arraytostring) == 0) {    // If user successfully enters correct sequence
                stagesCorrect++;
                    if (lives < 3) {
                        lives++;
                        
                        
                        livesgained++;   
                        
                    }
                    update_rgb_colour();
                    printf("\n-------Correct!------\n");
                    wipeUserInput();
                } else {    // User did not successfully enter correct sequence
                    printf("\n--------?-------\n");
                    wipeUserInput();
                    lives--;
                    liveslost++;
                    proceed_to_next_level = false;
                    update_rgb_colour();
                    break;
                }
            }
            if (proceed_to_next_level){
                stagePercent = (stagesCorrect / stagesTotal ) * 100;
                printf("Total Stages Correct: %f\n", stagesCorrect);
                printf("Total Stages:         %f\n", stagesTotal);
                printf("Percentage:           %f%%\n", stagePercent);
                printf("\n---------------------------------------\n");
                stagePercent = 0;
                stagesTotal = 0;
                stagesCorrect = 0;
                level++;
            }
        } else {
            // Game over, player wins
            print_player_wins_screen();
            return 0;
        }
    }

    print_game_over_screen();

    return 0;                      // Application return code
}