#include <stdio.h>

// Case 1: Duplicate code in if-else branches
// Rationale: Removing duplicate code from
// if-else statements, where the code resides
// in both branches. This, for example, could
// be moved outside of the if-else.
void ifAndElse(int n) {
  if (n < 10) {       // Increment ifBranchNo counter -> ifBranchNo = 1
    printf("Oh no!");
  }
  if (n > 10) {       // Increment ifBranchNo counter -> ifBranchNo = 2
    printf("Hey!");
  } else {            // Use current ifBranchNo -> elseBranchNo = ifBranchNo = 2
    printf("Hey!");
  }
}

int main() {
  ifAndElse(20);
}

/* Will be translated into:
{
  'call': {
    'ifAndElse': {
      {
        function: 'main',
        arguments: [20],
        ifBranchNo: false,
        elseBranchNo: false,
      },
    },
    'printf': {
      {
        function: 'ifAndElse',
        arguments: ['Oh no!'],
        ifBranchNo: 1,
        elseBranchNo: false
      },
      {
        function: 'ifAndElse',
        arguments: ['Hey!'],
        ifBranchNo: 2,
        elseBranchNo: false
      },
      {
        function: 'ifAndElse',
        arguments: ['Hey!'],
        ifBranchNo: false,
        elseBranchNo: 2
      }
    }
  },
  'variable': {}
}
*/

/* The pass can then look at the analyses:
if (calledFunction1 == calledFunction1 &&
    arguments1 == arguments2 &&
    ifBranchNo == elseBranchNo) {
  printf("There is an identical code block in both the if and else branch.");
}
*/


/* Relevant concerns
2. Does your approach work also if there are several statements in the then and else block?
  Yes, it handles that automatically since it looks at all the statements.

3. Can you find two identical if-then-else in two different methods?
  No :( The information is there, but there needs to be a more thorough analyses for that.
*/
