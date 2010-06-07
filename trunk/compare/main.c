#include<stdio.h>
#include<string.h>

#define CUSHION 5

typedef enum{false, true} bool;

// prototypes
static int bruteMatch(char *pTarget, char *pSource, int targetLength, int sourceLength);
static bool recursiveMatch(char *pTarget, char *pSource, int targetLength, int sourceLength, int targetIndex, int sourceIndex, int matches);
static bool gapMatch(char *pTarget, char *pSource, int targetLength, int sourceLength);

// number of matches variable
int numMatches;

int main()
{
    // Local variables.
	char word[256];
	char sentence[256];
	int word_length;
	int sentence_length;
    bool result = false;

    // Initialize the number of matches.
    numMatches = 0;

	// Retrieve user input.
	printf("Enter target:\n");
	scanf("%s", word);
	word_length = strlen(word);
	getchar();
	printf("Enter source:\n");
	scanf("%s", sentence);
	sentence_length = strlen(sentence);
	printf("\n");

	// Perform search/comparison here.
	//bruteMatch(word, sentence, word_length, sentence_length);
    result = recursiveMatch(word, sentence, word_length, sentence_length, 0, 0, 0);
    if (result)
        printf("Found target in source.\n");
    else
        printf("Didn't find target in source.\n");

	return 0;
} // end main

/* brute force algorithm */
int bruteMatch(char *pTarget, char *pSource, int targetLength, int sourceLength)
{
    int tPos = 0;
    int sPos = 0;
    int tTemp = 0;
    int sTemp = 0;
    int counter = 0;
    bool foundMatch = false;

    // continue searching...
    do{
        while (tTemp < targetLength) // turn into a while?
        {
            // comparing two chars
            if (*(pTarget+tTemp) != *(pSource+sTemp))
            {
                // jumping up in target
                tTemp++;
            }
            else
            {
                // match found
                foundMatch = true;
                break;
                // or goto Compare;
            }
        }
        if (!foundMatch) // target in source didn't match any chars in the target, so take a step in source
        {
            sTemp++;
            tTemp = 0;
            // jump up in source
        }
        else // a match was found and we jumped out of loop and landed here
        {
            // set the tPos and sPos
            tPos = tTemp;
            sPos = sTemp;

            // loop through until the chars don't match anymore
            counter = 0;
            numMatches = 0;
            while (*(pTarget + tTemp + counter) == *(pSource + sTemp + counter))
            {
                numMatches++;
                counter++;
                if (numMatches > CUSHION)
                {
                    // matches exceed cushion - likely pattern emerged
                    //printf("Number of matches exceeded the cushion.\n");

                }
            }

            // reset search vars
            tTemp = 0;
            sTemp = sPos + 1;
        }
    }while (sTemp < sourceLength); // only go through this comparison until the source has been fully checked with the target...

    return 0;
} // end bruteMatch

/* Recursive gap algorithm */
bool recursiveMatch(char *pTarget, char *pSource, int targetLength, int sourceLength, int targetIndex, int sourceIndex, int matches)
{
    bool result = false; // assume false to start

    if (matches > CUSHION)
        return true; // an acceptable number of matches was found
    else if (targetIndex > targetLength)
        return false; // exceeded the bounds
    else // perform search
    {
        while (sourceIndex < sourceLength)
        {
            //printf("comparing %c and %c\n", pTarget[targetIndex], pSource[sourceIndex]);
            if (pTarget[targetIndex] == pSource[sourceIndex])
                /*result =*/ return recursiveMatch(pTarget, pSource, targetLength, sourceLength, targetIndex + 1, sourceIndex + 1, matches + 1);
            else
            {
                sourceIndex++;
                if (sourceIndex >= sourceLength)
                    return false; // exceeded bounds
            }
        }

    }

    return result;
}

/* Iterative gap algorithm */
bool gapMatch(char *pTarget, char *pSource, int targetLength, int sourceLength)
{
    // TODO: implement with the use of a stack
    bool result = false;
    int matches = 0; // number of consecutive matches
    int tIndex = 0; // target index
    int sIndex = 0; // source index
    int stackIndex = 0; // location into the stack array
    int stack[sourceLength]; // stack to hold locations of consecutive chars

    while (tIndex < targetLength && sIndex < sourceLength) {

        // chekc for matches and advance as appropriate
        if (pTarget[tIndex] == pSource[sIndex]) {
            stack[stackIndex++] = tIndex;
            tIndex++;
            sIndex++;
            matches++;
        } else {
            sIndex++;
            tIndex = stack[stackIndex--];
        }

        // check to see if matches exceeds threshold
        if (matches > CUSHION) {
            result = true;
        }
    }

    return result;
}
