/*********************************************************************
** Program name: keygen (HW4)
** Author: Charles Ledbetter
** Date: 3/14/2018
** Description: A simple program for creating random character
sequences for use as one time pads. Insure that the date and time of
creation is kept secret for perfect secrecy.
*********************************************************************/
#include<stdio.h>
#include<stdlib.h>
#include<time.h>

//take a number and make that many random chars
int main(int argc, char* argv[]){
  // for loop control
  int i = 0;

  // for char randomization
  char rando = '\0';

  //convert arg 1 into an integer
  int nLength = atoi(argv[1]);

  // initialize random numbers
  srand(time(0));

  //print the number of characters asked for
  for(i = 0; i < nLength; i++){
    //use rand to generate capital letter key chars
    rando = rand() % 27 + 65;

    //this is a workaround for adding spaces
    if(rando == '['){
      rando = ' ';
    }

    //print to screen
    printf("%c", rando);
  }
  //add a newline
  printf("\n");

  return 0;
}
