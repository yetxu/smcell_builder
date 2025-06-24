#ifndef DB_CELL_LIST_H_
#define DB_CELL_LIST_H_
#include<stdint.h>
#include<queue>
#include"mutex.h"

#define ETIMED_OUT 0 //for function pthread_cond_timedwait

#define CELL_SIZE 491 //cell size 

//the minimum number of cell in queue that can be read by process thread, we set a minimum number to avoid frequently exchanging 
//double-buffer between write thread and read thread
#define MIN_READ_CELL_NUM 10

//maximum time span between successive read and write ptr exchange, in order to 
//avoid the read buffer stays empty and write buffer stays not too full to be read (which is caused by MIN_READ_CELL_NUM)
#define MAX_EXCHANGE_TIME_SPAN 20 //20 seconds

//mutex lock to lock idle_list in receive and process threada, according to result of experiment, use LOCK_IDLE will not cause
//performance loss obviously, so use it defaultly
#define LOCK_IDLE   

using namespace Mutex;

struct Cell{
    uint32_t cell_id;

    uint32_t unicast_ip;    //unicast ip, this is not necessary, 4 bytes len
    
    int data_len;           //data length of cell_body

    int body_start_pos;     //the cell body's start position

    char cell_data[CELL_SIZE];        //pointer to the buffer where store cell body

    Cell():unicast_ip(0), data_len(0), body_start_pos(0){
    };

    ~Cell(){
    }
};

//function pointer that use Cell* as a function parameter
//used in PopAndProcsCell function
typedef void(* ProcsCellFunc)(Cell*);  

class DbCellList{
friend void OnTimer(void* param);
public:
    DbCellList();
    ~DbCellList();

    //initialize cell list by allocating some Cells at beginning of program
    //to avoid frequently allocating Cell 
    void Init(int init_cell_num);

    //in receive thread, push received udp pack into the cell list, 
   void PushPacket(char* udp_packet, int len);

   //in process thread,  pop cell struct from std::queue, and process the cell
   //remember after use the Cell pointer this function returned, delete it
   //
   //or we can call PopAndProcsCell function to avoid to recycle cell* manually
   Cell* PopWorkCell(); 

   //pop idle cell
   Cell* PopIdleCell();

   //push idle cell into idle_list
   void RecycleCell(Cell*);

   //pop and process cell, when call this function, give a function pointer pointes
   //to function that process the newly popped cell, after processed, the cell should be enqueued into idle_list
   void PopAndProcsCell(ProcsCellFunc fp);

   void ExchangeRwPtr();
private:

    //use double-buffer mechanism to exchange data between receive thread and process thread  
    //in all, we have three std::queue<cell*>(s), a idle_list which contains the post-processed cell, work_list1 and work_list2, contains the received cell.
    //work_list1 and work_list2 is the realization of double-buffer mechanism, one of the two work_lists is possessed and written by receive thread, the other 
    //is possessed and read by process thread.
    //
    //In receive thread, when we receive a packet, load its data into a cell popped from idle_list perferentially, if idle_list is empty, we allocate 
    //a Cell* to load packet data. After load data, cell is pushed into work_list1 or work_list2, it depends on which work list is being write by receive thread.
    //
    //In process thread, we try to pop cell from work_lsit1 or work_list2, it depends on which work list is being read by process thread.
    //For example, if work_list1 is possessed and read by process thread, we pop cell* from work_list1 until it's empty, then if work_list2(which is being possessed
    //and written by receive thread) is not empty, we exchange work_list1 and work_list2 by change  pointer (of type std::queue<Cell*>* ) write_list_ptr_ 
    //and read_list_ptr_. Then in process thread, we continue to pop cell from work_list2 , and in receive thread, we push newly-received cell into work_list1....
    //
    //use double-buffer mechanism, we only need to lock the shared queue at very short time (when we exchange the read queue and write queue), that will reduce a lot
    //of time-consuming caused by mutex lock in multi-thread program
    //

   typedef std::queue<Cell*> CellQueue;
   typedef std::queue<Cell*>* CellQueuePtr;
    //queue contains cells newly-received in receive thread or waited to be processed in process thread
    CellQueue work_list1_;   

    //queue contains cells newly-received in receive thread or waited to be processed in process thread
    CellQueue work_list2_;   

    //queue contains cells which has been processed and is waiting to load new data and be enqueued in work_list
    CellQueue idle_list_;   

    //pointer pointing to the work list being written by receive thread
    CellQueuePtr write_list_ptr_; 

    //pointer pointing to the work list being read by process thread
    CellQueuePtr read_list_ptr_;   

   volatile bool read_list_empty_;
   MutexLock mutex_lock_work_;
   Condition condition_;
#ifdef LOCK_IDLE
   MutexLock mutex_lock_idle_;
#endif
};
#endif
