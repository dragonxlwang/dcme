#ifndef CONSTANTS
#define CONSTANTS

// The following parameters are set in order to have variables on stack
#define NUP 200                // upper bound for N  (embedding dimension)
#define KUP 200                // upper bound for K  (dual dimension)
#define VUP 0xFFFFF            // upper bound for V (vocabulary)
#define QUP 100                // upper bound for micro maximal entropy
#define WUP TEXT_MAX_WORD_LEN  // upper bound for character number in a word
#define SUP TEXT_MAX_SENT_WCT  // upper bound for word number in a sentence

#endif /* ifndef CONSTANTS */
