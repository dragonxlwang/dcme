#ifndef CONSTANTS
#define CONSTANTS

// The following parameters are set in order to have variables on stack
#define NUP 0x1FFFF            // upper bound for N  (feature dimension)
#define KUP 200                // upper bound for K  (dual dimension)
#define QUP 100                // upper bound for micro maximal entropy
#define WUP TEXT_MAX_WORD_LEN  // upper bound for character number in a word
#define SUP TEXT_MAX_SENT_WCT  // upper bound for word number in a sentence
#define LUP 0xFFFF             // upper bound for a sentence length (abstract)
#define CUP 101010             // upper bound for number of classes
#endif /* ifndef CONSTANTS */
