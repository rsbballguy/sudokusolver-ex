#include "sSolver.h"
#include <stdio.h>
#include <conio.h>
#include <iostream>

using namespace std;

gridInfo board;

rowEvt rEvt[MAX_ROW];
colEvt cEvt[MAX_COL];
blockEvt bEvt[(MAX_COL*MAX_ROW)/(BLOCK_SIZE*BLOCK_SIZE)];
HANDLE tHandles[MAX_ROW*MAX_COL];

void display()
{
    WaitForSingleObject(mutexEx,INFINITE);
    system("cls");
    cout<<"\n";
    for(int i =0 ;i<MAX_ROW;i++)
    {
        for(int j =0;j<MAX_COL;j++)
        {
            if(board.grid[i][j].value)
            {
                cout<<board.grid[i][j].value<<" ";
            }
            else
            {
                cout<<" "<<" ";
            }
        }
        cout<<"\n";
    }
    Sleep(400);
    ReleaseMutex(mutexEx);
}

bool validateAndInsert(cellAddr addr , int number)
{
    int rStart,cStart;
    rStart = (addr.bVal / BLOCK_SIZE)*BLOCK_SIZE;
    cStart = (addr.bVal % BLOCK_SIZE)*BLOCK_SIZE;

    //Update Row
    for(int i =0;i< MAX_ROW ; i++)    
        if(i != addr.rVal)
            if(number == board.grid[i][addr.cVal].value)
                return false;


    //update Col
    for(int i =0;i< MAX_COL ; i++)    
        if(i != addr.cVal)
            if(number == board.grid[addr.rVal][i].value)
                return false;


    //Update block
    for(int i = rStart;i<rStart + BLOCK_SIZE;i++)
        for(int j = cStart;j < cStart + BLOCK_SIZE;j++)
            if((i != addr.rVal)&&(j != addr.cVal))
                if(number == board.grid[i][j].value)
                    return false;

    return board.grid[addr.rVal][addr.cVal].setValue(number);
}

void notifyAllThreads(cellAddr addr, int number)
{
    int rStart,cStart;
    int i,j;
    rStart = (addr.bVal / BLOCK_SIZE)*BLOCK_SIZE;
    cStart = (addr.bVal % BLOCK_SIZE)*BLOCK_SIZE;

    //Update Row
    for( i =0;i< MAX_ROW ; i++)    
        if(i != addr.rVal)
            board.grid[i][addr.cVal].eraseValue(number);

    PulseEvent(rEvt[addr.rVal]);
        
    //update Col
    for( i =0;i< MAX_COL ; i++)    
        if(i != addr.cVal)
            board.grid[addr.rVal][i].eraseValue(number);

    PulseEvent(cEvt[addr.cVal]);
    
    //Update block
    for( i = rStart;i<rStart + BLOCK_SIZE;i++)
        for( j = cStart;j < cStart + BLOCK_SIZE;j++)
            if((i != addr.rVal)&&(j != addr.cVal))
                board.grid[i][j].eraseValue(number);
    
    PulseEvent(bEvt[addr.bVal]);
}

bool rowUpdate(cellAddr addr)
{
    int rowVal = addr.rVal;
    cell& myCell = board.grid[addr.rVal][addr.cVal];

    for (int i =0;i<MAX_ROW;i++)
    {
        if( (i != addr.cVal)&&
            (board.grid[rowVal][i].value!= 0))
        {
            if(true == myCell.eraseValue(board.grid[rowVal][i].value))
            {
                notifyAllThreads(addr, board.grid[addr.rVal][addr.cVal].value);
                return true;
            }
        }
    }
    return false;
}

bool colUpdate(cellAddr addr )
{
    int colVal = addr.cVal;
    cell& myCell = board.grid[addr.rVal][addr.cVal];

    for (int i =0;i<MAX_COL;i++)
    {
        if( (i != addr.cVal)&&
            (board.grid[i][colVal].value!= 0))
        {
            if(true == myCell.eraseValue(board.grid[i][colVal].value))
            {
                notifyAllThreads(addr, board.grid[addr.rVal][addr.cVal].value);
                return true;
            }
        }
    }
    return false;
}

bool blockUpdate(cellAddr addr )
{
    int blockVal = addr.bVal;
    cell& myCell = board.grid[addr.rVal][addr.cVal];

    int startRow,startCol;

    startRow = (blockVal /BLOCK_SIZE)*BLOCK_SIZE;
    startCol = (blockVal % BLOCK_SIZE)*BLOCK_SIZE;

    for(int i = startRow;i<startRow+BLOCK_SIZE;i++)
    {
        for(int j=startCol;j<startCol+BLOCK_SIZE;j++)
        {
            if( ((i != addr.rVal) || (j!= addr.cVal)) &&
            (board.grid[i][j].value!= 0))
            {
                if(true == myCell.eraseValue(board.grid[i][j].value))
                {
                    notifyAllThreads(addr, board.grid[addr.rVal][addr.cVal].value);
                    return true;
                }
            }
        }
    }
    return false;
}

void initialize()
{
    char strTmp[100];
    board.solved = false;
    board.solved_cells = 0;
    board.unsolved_cells = 0;
    board.threadCount =0;
    memset(strTmp ,0 ,sizeof(strTmp));
    int nBlocks = (MAX_ROW*MAX_COL)/(BLOCK_SIZE*BLOCK_SIZE);

    for(int i = 0;i<MAX_ROW;i++)
    {
        for(int j = 0;j<MAX_COL;j++)
        {
            board.grid[i][j].value = 0;
            board.unsolved_cells++;
            for(int k =0;k<((MAX_COL*MAX_ROW)/nBlocks);k++)
                board.grid[i][j].list.insert(k+1);
            sprintf(strTmp,"xcellMutex_%d_%d",i,j);
            board.grid[i][j].cellMutex = CreateMutex(NULL, false,strTmp);
        }
    }
    for(int i = 0; i<MAX_ROW; i++)
    {
        sprintf(strTmp ,"rowEvent_%d",i);
        rEvt[i] = CreateEvent(NULL,false,false,
            strTmp);
        sprintf(strTmp ,"colEvent_%d",i);
        cEvt[i] = CreateEvent(NULL,false,false,
            strTmp);
        sprintf(strTmp ,"blockEvent_%d",i);
        bEvt[i] = CreateEvent(NULL,false,false,
            strTmp);
    }
    mutexEx = CreateMutex(NULL,false,
        "MyMutex");
}

DWORD WINAPI  cellThread(LPVOID args)
{
    cellAddr *addr;
    addr = (cellAddr*)args;
    
    DWORD retVal =0;
    HANDLE waitList[3];
    waitList[0] = rEvt[addr->rVal];
    waitList[1] = cEvt[addr->cVal];
    waitList[2] = bEvt[addr->bVal];

    board.threadCount++;
    board.grid[addr->rVal][addr->cVal].threadAlive = true;

    if(board.grid[addr->rVal][addr->cVal].value > 0)
    {
        PulseEvent(rEvt[addr->rVal]);
        PulseEvent(cEvt[addr->cVal]);
        PulseEvent(bEvt[addr->bVal]);
        WaitForSingleObject(mutexEx,INFINITE);
        cout<<"\nCell_Thread <"<<addr->rVal<<"_"<<addr->cVal<<"_"
            <<addr->bVal<<"> Updated ";
        ReleaseMutex(mutexEx);
        board.solved_cells++;
        board.unsolved_cells--;
        board.threadCount--;
        board.grid[addr->rVal][addr->cVal].threadAlive = false;
        return true;
    }

    while(1)
    {
        retVal =WaitForMultipleObjects(3,
            waitList ,
            false,
            INFINITE);
        switch(retVal)
        {
            case WAIT_OBJECT_0:
                PulseEvent(rEvt[addr->rVal]);               
                break;
            case (WAIT_OBJECT_0 +1):
                PulseEvent(cEvt[addr->cVal]);
                break;
            case (WAIT_OBJECT_0 +2):
                PulseEvent(bEvt[addr->bVal]);
                break;
            default:
                WaitForSingleObject(mutexEx,INFINITE);
                cout<<"\nCell_Thread <"<<addr->rVal<<"_"<<addr->cVal<<"_"
                    <<addr->bVal<<"> Failed";
                ReleaseMutex(mutexEx);    
                board.threadCount--;
                board.grid[addr->rVal][addr->cVal].threadAlive = false;
                return false;
        }
        if(rowUpdate(*addr)||colUpdate(*addr) || blockUpdate(*addr))
        {
            PulseEvent(rEvt[addr->rVal]);
            PulseEvent(cEvt[addr->cVal]);
            PulseEvent(bEvt[addr->bVal]);
            WaitForSingleObject(mutexEx,INFINITE);
            cout<<"\nCell_Thread <"<<addr->rVal<<"_"<<addr->cVal<<"_"
                    <<addr->bVal<<"> Updated ";
            ReleaseMutex(mutexEx);
            board.solved_cells++;
            board.unsolved_cells--;
            board.threadCount--;
            board.grid[addr->rVal][addr->cVal].threadAlive = false;
            //display();
            return true;
        }
    }
}

void fillGrid()
{
#if 0
    int t[9][9]={
        {0,2,0,0,0,0,0,9,0},
        {0,0,0,0,1,6,2,0,0},
        {6,5,1,4,2,0,0,0,0},
        {0,0,0,0,6,0,0,7,0},
        {9,3,0,0,0,0,0,1,8},
        {0,6,0,0,8,0,0,0,0},
        {0,0,0,0,7,5,0,4,0},
        {0,0,2,8,9,0,0,0,0},
        {0,9,0,0,0,0,0,3,0}
    };
#else
    int t[9][9]={
        {0,0,0,8,0,2,0,0,0},
        {0,1,0,0,0,0,4,7,0},
        {0,0,0,0,1,0,8,5,6},
        {0,0,7,1,0,0,6,0,0},
        {0,8,3,0,0,0,2,4,0},
        {0,0,6,0,0,9,1,0,0},
        {9,3,1,0,4,0,0,0,0},
        {0,5,8,0,0,0,0,9,0},
        {0,0,0,9,0,0,0,0,0}
    };
#endif
    /*int t[3][3]={
        {1,2,3},
        {4,0,6},
        {7,5,9}};*/
    /*int t[4][4]={
        {0,2,0,4},
        {0,0,1,0},
        {2,0,4,3},
        {0,3,0,1}
    };*/
    
    for(int i =0;i<MAX_ROW;i++)
    {
        for(int j =0;j<MAX_COL;j++)
        {
            board.grid[i][j].value=t[i][j];
            if(t[i][j])
            {
                board.grid[i][j].list.clear();
                board.grid[i][j].list.insert(board.grid[i][j].value);
            }
        }
    }
}
int colValidate(int col)
{
    std::set<int> lcount;
    bool fixFlag = false;
    cellAddr addr;
    

    for(int number = 1;number<=9;number++)
    {
        fixFlag = false;
        for(int row =0;row<MAX_ROW;row++)
        {
            if(board.grid[row][col].list.end() != 
                board.grid[row][col].list.find(number))
            {
                if(number == board.grid[row][col].value)
                {
                    fixFlag = true;
                    break;
                }
                lcount.insert(row);
            }
        }
        if(fixFlag)
            continue;
        if(1 == lcount.size())
        {
            addr.rVal = *lcount.begin();
            addr.cVal = col;
            addr.bVal = ((addr.rVal/BLOCK_SIZE)*BLOCK_SIZE)+(col/BLOCK_SIZE);
            if(!board.grid[addr.rVal][addr.cVal].value)
            {                    
                if(true == validateAndInsert(addr, number))
                    notifyAllThreads(addr ,number);
            }
        }
        lcount.clear();
    }
    return 0;
}
int rowValidate(int row)
{
    std::set<int> lcount;
    bool fixFlag = false;
    cellAddr addr;
    

    for(int number = 1;number<=9;number++)
    {
        fixFlag = false;
        for(int col =0;col<MAX_COL;col++)
        {
            if(board.grid[row][col].list.end() != 
                board.grid[row][col].list.find(number))
            {
                if(number == board.grid[row][col].value)
                {
                    fixFlag = true;
                    break;
                }
                lcount.insert(col);
            }
        }
        if(fixFlag)
            continue;
        if(1 == lcount.size())
        {
            addr.rVal = row;
            addr.cVal = *lcount.begin();
            addr.bVal = ((row/BLOCK_SIZE)*BLOCK_SIZE)+(addr.cVal/BLOCK_SIZE);
            if(!board.grid[addr.rVal][addr.cVal].value)
            {                    
                if(true == validateAndInsert(addr, number))
                    notifyAllThreads(addr ,number);
            }
        }
        lcount.clear();
    }
    return 0;
}
int blockValidate(int block)
{
    cellAddr addr;
    addr.bVal = block;
    int rVal;
    bool setEvent;
    bool fixFlag;
    int cVal;
    unsigned int row,rstart,col,cstart;
    std::set<int> lrow;
    std::set<int> lcol;
    std::set<int>::iterator lit;

    int number=0;
    
    rstart = (block / BLOCK_SIZE)*BLOCK_SIZE;
    cstart = (block % BLOCK_SIZE)*BLOCK_SIZE;

    for(number = 1;number<=9;number++)
    {
        fixFlag = false;
        lrow.clear();
        lcol.clear();

        for(row = rstart;row < rstart+BLOCK_SIZE;row++)
        {
            for(col = cstart;col<cstart+BLOCK_SIZE;col++)
            {
                if(board.grid[row][col].list.end() != 
                    board.grid[row][col].list.find(number))
                {
                    if(number == board.grid[row][col].value)
                    {
                        fixFlag = true;
                        break;
                    }
                    lrow.insert(row);
                    lcol.insert(col);
                }
            }
            if(fixFlag)
                break;
        }
        if(fixFlag)
            continue;
        if(1 == lrow.size() )
        {
            if((1 == lrow.size()) &&
                (1 == lcol.size()))
            {
                addr.rVal = *(lrow.begin());
                addr.cVal = *(lcol.begin());
                addr.bVal = block;
                if(!board.grid[addr.rVal][addr.cVal].value)
                {                    
                    if(true == validateAndInsert(addr, number))
                        notifyAllThreads(addr ,number);
                }
            }
            setEvent = true; 
            lit = lrow.begin();
            rVal = *lit;
            lit++;            
            while(lit != lrow.end()){
                if(*lit != rVal)
                {
                    setEvent = false;
                    break;
                }
                lit++;
            }
            if(setEvent)
            {
                for(unsigned int range = 0 ;range<MAX_COL;range++ )
                {
                    if((range < cstart) || 
                        (range > (cstart+BLOCK_SIZE)))
                    {
                        board.grid[rVal][range].eraseValue(number);
                    }
                }
                PulseEvent(rEvt[rVal]);
                setEvent = false;
            }
        }
        if(1 == lcol.size())
        {
            setEvent = true; 
            lit = lcol.begin();
            cVal = *lit;
            lit++;            
            while(lit != lcol.end()){
                if(*lit != cVal)
                {
                    setEvent = false;
                    break;
                }
                lit++;
            }
            if(setEvent)
            {
                for(unsigned int range = 0 ;range<MAX_ROW;range++ )
                {
                    if((range < rstart) || 
                        (range > (rstart+BLOCK_SIZE)))
                    {
                        board.grid[range][cVal].eraseValue(number);
                    }
                }
                PulseEvent(cEvt[cVal]);
                setEvent = false;
            }
        }


    }
    return 0;
}

int getListOfhandles()
{
    int tCount =0;
    for(int i =0;i< MAX_ROW;i++)
    {
        for(int j = 0;j< MAX_COL;j++)
        {
            if(board.grid[i][j].threadAlive)
            {
                tHandles[tCount++] = board.grid[i][j].tHandle;
            }
        }
    }
    return tCount;
}
void main()
{
    initialize();
    fillGrid();
    DWORD retval = 0;
    int timeOutRetry =0 ;
    //DWORD tIds[MAX_ROW][MAX_COL];
    cellAddr cAddr[MAX_ROW][MAX_COL];
    //HANDLE waitList[81];
    display();
    int hres;

    for(int i =0;i<MAX_ROW;i++)
    {
        for(int j =0;j<MAX_COL;j++)
        {
            cAddr[i][j].cVal = j;
            cAddr[i][j].rVal = i;
            cAddr[i][j].bVal = ((i/BLOCK_SIZE)*BLOCK_SIZE + 
                (j/BLOCK_SIZE));

            board.grid[i][j].tHandle = CreateThread(NULL,
                0,
                cellThread,
                &cAddr[i][j],
                0,
                &board.grid[i][j].tId);
        }
    }


    do{

        getListOfhandles();
        retval = WaitForMultipleObjects(board.threadCount,
            tHandles, false,3000);
        if((retval >= WAIT_OBJECT_0)&&
            (retval <= (WAIT_OBJECT_0 + (MAX_ROW*MAX_COL))))
        {
            cellAddr caddr;
            caddr.rVal = (retval-WAIT_OBJECT_0)/(MAX_COL);
            caddr.cVal = (retval-WAIT_OBJECT_0)%(MAX_COL);
            caddr.bVal = ((caddr.rVal/BLOCK_SIZE)*BLOCK_SIZE + 
                    (caddr.cVal/BLOCK_SIZE));

            PulseEvent(rEvt[caddr.rVal]);
            PulseEvent(cEvt[caddr.cVal]);
            PulseEvent(bEvt[caddr.bVal]);
        }
        for(int index1 =0 ;index1 < (BLOCK_SIZE * BLOCK_SIZE);index1++)
        {
            rowValidate(index1);
            colValidate(index1);
            blockValidate(index1);
        }
        if(retval == WAIT_FAILED)
        {
            hres= GetLastError();
            cout<<"\n Wait Failed!";
            //display();
        }
        if(retval == WAIT_TIMEOUT)
        {
            //display();
            timeOutRetry++;
            cout<<"\n Wait Timed out!";
            if(timeOutRetry > 5)
                break;
        }
    
        
    
        //WaitCount = WaitCount-1;
    }while(board.unsolved_cells);
    
    display();
    return;
}