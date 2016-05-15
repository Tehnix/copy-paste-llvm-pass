#include <stdio.h>


// Case 2: Moving code to constant
// Rationale: Sometimes duplicate code could instead
// be a constant. Instead of defining 'a' in main and
// 'c' in additionWithConstant, it could be moved out
// into a global constant.
int additionWithConstant(int a, int b) {
  int c = 100 * 5;
  return a + b + c;
}

int main() {
  int a = 100 * 5;
  additionWithConstant(5, 10);
}

/* Will be translated into
{
  'call': {
    'additionWithConstant': {
      {
        function: 'main',
        arguments: [5, 10],
        ifBranchNo: false,
        elseBranchNo: false
      }
    }
  },
  'variable': {
    '100 * 5': {
      {
        function: 'main',
        lhs: 'a'
      },
      {
        function: 'additionWithConstant',
        lhs: 'c'
      }
    }
  }
}
*/

/* The pass can then look at the analyses:
if (rhs1 == rhs2) {
  printf("The variable '\{lhs1}' from '\{function1}' is identical to '\{lhs1}' from '\{function1}'. Consider moving them out into a constant.");
}
*/
