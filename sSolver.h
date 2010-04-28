#include <windows.h>
#include <set>
#include <iostream>

typedef HANDLE rowEvt;
typedef HANDLE colEvt;
typedef HANDLE blockEvt;

HANDLE mutexEx;

#define MAX_ROW 9
#define MAX_COL 9
#define BLOCK_SIZE 3

typedef std::set<int> prob_list;

struct cell{
    int value;
    prob_list list;
    HANDLE cellMutex;
    bool threadAlive;
    DWORD tId;
    HANDLE tHandle;
public: 
    bool eraseValue(int number)
    {
        if((1 == list.size())&&
            (number == *list.begin()))
        {
            value = *list.begin();
            return true;
        }
        WaitForSingleObject(cellMutex,INFINITE);
        list.erase(number);
        ReleaseMutex(cellMutex);
        if(1 == list.size())
        {
            value = *list.begin();
            return true;
        }
        return false;
    }
    bool setValue(int number)
    {        
        WaitForSingleObject(cellMutex,INFINITE);
        list.clear();
        list.insert(number);
        ReleaseMutex(cellMutex);
        value = number;
        return true;
    }
};

struct cellAddr{
    int rVal;
    int cVal;
    int bVal;
};


struct gridInfo{
    cell grid[MAX_ROW][MAX_COL];
    int threadCount;
    int solved_cells;
    int unsolved_cells;
    bool solved;
};

int blockValidate(int block);