#include <iostream>
#include <string>
#include <csignal>
#include <sstream>
#include <fstream>

using namespace std;

void sighandler(int)
{
throw 99;
}

int main()
{
signal(SIGINT, sighandler);
float matrix[6][6];
float num1;
float num2;
float num3;
float num4;
float num5;
float num6;
char lastLine[100];
std::cout << "Reading 'matrix.txt'..." << std::endl;
std::string line;
ifstream infile;
infile.open("matrix.txt");
for (int i=0; i<6; i++)
{
infile.getline(lastLine,100);
sscanf(lastLine, "%f, %f, %f, %f, %f, %f, ", &num1,&num2,&num3,&num4,&num5,&num6);
std::cout << num1 << num2 << num3 << std::endl;
matrix[i][0]=num1;
matrix[i][1]=num2;
matrix[i][2]=num3;
matrix[i][3]=num4;
matrix[i][4]=num5;
matrix[i][5]=num6;
}
infile.close();

return 0;
}
