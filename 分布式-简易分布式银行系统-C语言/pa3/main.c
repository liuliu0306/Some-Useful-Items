#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>
#include <fcntl.h>

#include "banking.h"
#include "common.h"
#include "ipc.h"
#include "pa2345.h"

#define LENGTH 2048

int16_t local_time = 0;

timestamp_t get_lamport_time() {
    return local_time;
}

timestamp_t add_lamport_time() {
    local_time++;
    return local_time;
}

timestamp_t set_lamport_time(int16_t time) {
    local_time = time + 1;
    return local_time;
}

void transfer(void* parent_data, local_id src, local_id dst, balance_t amount) {
    printf("Tranfer %d to %d\n", src, dst);
    int(*pd)[2][2] = parent_data;
    char* buffer = (char*)malloc(LENGTH);

    int cur_t = add_lamport_time();
    MessageHeader* header = (MessageHeader*)malloc(sizeof(MessageHeader));
    TransferOrder* transfer = (TransferOrder*)malloc(sizeof(TransferOrder));
    header->s_local_time = cur_t;
    header->s_type = TRANSFER;
    header->s_magic = MESSAGE_MAGIC;
    header->s_payload_len = sizeof(TransferOrder);
    transfer->s_src = src;
    transfer->s_dst = dst;
    transfer->s_amount = amount;
    *(MessageHeader*)(buffer) = *header;
    *(TransferOrder*)(buffer + 8) = *transfer;

    int result = write(pd[src - 1][0][1], buffer, sizeof(MessageHeader) + sizeof(TransferOrder));
    printf("Parent process tranfered %d to %d\n", src, dst);
    MessageHeader* rec_ack = (MessageHeader*)malloc(sizeof(MessageHeader));
    while (1) {
        result = read(pd[dst - 1][1][0], rec_ack, 8);
        if (result > 0 && rec_ack->s_type == ACK) {
            cur_t = add_lamport_time();
            if (cur_t <= rec_ack->s_local_time) {
                cur_t = set_lamport_time(rec_ack->s_local_time);
            }
            printf("Recieve ack from %d\n", dst);
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    int N, i, j;
    if (argc < 3 || (argv[1][0] != '-' && argv[1][1] != 'p')) {
        printf("usage -p N balance*N\n");
        exit(0);
    }

    N = atoi(argv[2]);
    FILE* pFileEvent = fopen("events.log", "wt+");
    FILE* pFilePipes = fopen("pipes.log", "wt+");

    int* balance = (int*)malloc(sizeof(int) * N);
    for (i = 0; i < N; i++) {
        balance[i] = atoi(argv[3 + i]);
    }
    printf("N=%d\n", N);
    int fpid;
    int pipeline[N][N][2];
    int parent_pip[N][2][2];
    int ppid = getpid();


    printf("Initialization successful!\n");
    printf("---------------------Welcome to the banking system---------------------\n\n");
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            if (i == j) continue;
            pipe2(pipeline[i][j], O_NONBLOCK);
        }
        pipe2(parent_pip[i][0], O_NONBLOCK);
        pipe2(parent_pip[i][1], O_NONBLOCK);
    }
    int p_logic = -1;
    for (i = 0; i < N; i++) {
        fpid = fork();
        if (fpid == 0) {
            for (j = 0; j < N; j++) {
                if (i != j) {
                    close(pipeline[i][j][0]);
                    close(pipeline[j][i][1]);
                    close(parent_pip[j][0][0]);
                    close(parent_pip[j][0][1]);
                    close(parent_pip[j][1][0]);
                    close(parent_pip[j][1][1]);
                }
            }
            close(parent_pip[i][0][1]);
            close(parent_pip[i][1][0]);
            p_logic = i;
            break;
        }
    }

    if (fpid == 0) {
        for (i = 0; i < N; i++) {
            for (j = 0; j < N; j++) {
                if (i != j && i != p_logic && j != p_logic) {
                    close(pipeline[i][j][0]);
                    close(pipeline[i][j][1]);
                }
            }
        }
    }
    if (fpid != 0) {
        for (i = 0; i < N; i++) {
            for (j = 0; j < N; j++) {
                if (i != j) {
                    close(pipeline[i][j][0]);
                    close(pipeline[i][j][1]);
                }
            }
        }
        for (i = 0; i < N; i++) {
            close(parent_pip[i][0][0]);
            close(parent_pip[i][1][1]);
        }
    }

    /** TODO: nothing **/
    if (fpid == 0) {
        int stage = 0;
        int iter = 0;
        int self_balance = balance[p_logic];
        int* is_done = (int*)malloc(sizeof(int) * N);
        is_done[p_logic] = 1;
        BalanceHistory* bhistory = (BalanceHistory*)malloc(sizeof(BalanceHistory));
        bhistory->s_id = p_logic + 1;



        int cur_t = get_lamport_time();
        BalanceState* bstate = (BalanceState*)malloc(sizeof(BalanceState));
        bstate->s_balance = self_balance;
        bstate->s_time = cur_t;
        bstate->s_balance_pending_in = 0;
        bhistory->s_history[cur_t] = *bstate;
        while (MAX_T - get_physical_time()) {

            iter++;
            fprintf(pFileEvent, log_loop_operation_fmt, p_logic + 1, cur_t, cur_t);
            printf(log_loop_operation_fmt, p_logic + 1, cur_t, cur_t);

            if (stage == 0) {
                printf("\n----------%d: \"START\" -----------------\n", p_logic + 1);
                fprintf(pFilePipes, "\n----------%d: \"START\" -----------------\n", p_logic + 1);
                fprintf(pFileEvent, log_started_fmt, p_logic + 1, p_logic + 1, getpid(), ppid, self_balance);
                printf(log_started_fmt, p_logic + 1, p_logic + 1, getpid(), ppid, self_balance);
				
                MessageHeader* header = (MessageHeader*)malloc(sizeof(MessageHeader));
                header->s_local_time = add_lamport_time();
                header->s_magic = MESSAGE_MAGIC;
                header->s_type = STARTED;


                Message* msg = (Message*)malloc(sizeof(Message));

                //	"%d: process %1d (pid %5d, ppid %5d) has STARTED with balance $%2d\n";
                sprintf(msg->s_payload, log_started_fmt, p_logic + 1, p_logic + 1, getpid(), ppid, self_balance);
                header->s_payload_len = strlen(msg->s_payload);

                int length = strlen(msg->s_payload) + 8;
                msg->s_header = *header;
                for (i = 0; i < N; i++) {
                    if (i != p_logic) {
                        write(pipeline[p_logic][i][1], (char*)msg, length);
                    }
                }
                write(parent_pip[p_logic][1][1], (char*)msg, length);
                fprintf(pFilePipes, "%d: Sent message \"START\" to other process\n", p_logic + 1);

                int cur_t = get_lamport_time();
                BalanceState* bstate = (BalanceState*)malloc(sizeof(BalanceState));
                bstate->s_balance = self_balance;
                bstate->s_time = cur_t;
                bstate->s_balance_pending_in = 0;
                bhistory->s_history[cur_t] = *bstate;

                stage++;
            }
            else if (stage == 1) {
                // "Useful" job: waiting and processing TRANSFER and STOP messages

                int result = -1;
                char* buffer = (char*)malloc(LENGTH);
                MessageHeader* rec_header = (MessageHeader*)malloc(sizeof(MessageHeader));
                result = read(parent_pip[p_logic][0][0], (char*)rec_header, 8);

                if (result > 0) {
                    if (rec_header->s_type == TRANSFER) {
                        cur_t = add_lamport_time();
                        if (cur_t <= rec_header->s_local_time) {
                            for (int j = cur_t; j <= rec_header->s_local_time; j++) {
                                BalanceState* bstate = (BalanceState*)malloc(sizeof(BalanceState));
                                bstate->s_balance = self_balance;
                                bstate->s_time = j;
                                bstate->s_balance_pending_in = 0;
                                bhistory->s_history[j] = *bstate;
                            }
                            cur_t = set_lamport_time(rec_header->s_local_time);
                        }
                        TransferOrder* rec_tforder = (TransferOrder*)malloc(sizeof(TransferOrder));
                        result = read(parent_pip[p_logic][0][0], (char*)rec_tforder, sizeof(TransferOrder));

                        BalanceState* bstate = (BalanceState*)malloc(sizeof(BalanceState));
                        bstate->s_balance = self_balance - (rec_tforder->s_amount);
                        bstate->s_time = cur_t;
                        bstate->s_balance_pending_in = rec_tforder->s_amount;
                        bhistory->s_history[cur_t] = *bstate;

                        int dst = rec_tforder->s_dst;
                        cur_t = add_lamport_time();
                        rec_header->s_local_time = cur_t;


                        *(MessageHeader*)(buffer) = *rec_header;
                        *(TransferOrder*)(buffer + 8) = *rec_tforder;

                        self_balance -= rec_tforder->s_amount;
                        result = write(pipeline[p_logic][dst - 1][1], buffer, sizeof(MessageHeader) + sizeof(TransferOrder));
                        // log_transfer_out_fmt: "%d: process %1d transferred $%2d to process %1d\n";
                        bstate = (BalanceState*)malloc(sizeof(BalanceState));
                        bstate->s_balance = self_balance;
                        bstate->s_time = cur_t;
                        bstate->s_balance_pending_in = rec_tforder->s_amount;
                        bhistory->s_history[cur_t] = *bstate;
                        fprintf(pFileEvent, log_transfer_out_fmt, p_logic + 1, p_logic + 1, self_balance, dst);
                        fprintf(pFilePipes, "tranfer to %d\n", dst);
                        printf(log_transfer_out_fmt, p_logic + 1, p_logic + 1, self_balance, dst);

                    }
                    else if (rec_header->s_type == STOP) {
                        // Jump to stage2 => "stop" processing flow
                        fprintf(pFilePipes, "%d: Received \"STOP\" from parent process", p_logic + 1);
                        int cur_t = add_lamport_time();
                        if (cur_t <= rec_header->s_local_time)
                        {
                            for (int j = cur_t; j <= rec_header->s_local_time; j++)
                            {
                                BalanceState* bstate = (BalanceState*)malloc(sizeof(BalanceState));
                                bstate->s_balance = self_balance;
                                bstate->s_time = j;
                                bstate->s_balance_pending_in = 0;
                                bhistory->s_history[j] = *bstate;
                                printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>3  %d: write history at %d\n", p_logic + 1, j);
                            }
                            cur_t = set_lamport_time(rec_header->s_local_time);
                        }
                        BalanceState* bstate = (BalanceState*)malloc(sizeof(BalanceState));
                        bstate->s_balance = self_balance;
                        bstate->s_time = cur_t;
                        bstate->s_balance_pending_in = 0;
                        bhistory->s_history[cur_t] = *bstate;
                        printf("%d: Received \"STOP\" from parent process", p_logic + 1);
                        stage = 2;
                    }
                }
                for (i = 0; i < N; i++) {

                    result = -1;
                    if (i == p_logic) continue;
                    result = read(pipeline[i][p_logic][0], (char*)rec_header, 8);
                    int cur_t = 0;
                    if (result > 0) {
                        cur_t = add_lamport_time();
                        if (cur_t <= rec_header->s_local_time)
                        {
                            for (int j = cur_t; j <= rec_header->s_local_time; j++)
                            {
                                BalanceState* bstate = (BalanceState*)malloc(sizeof(BalanceState));
                                bstate->s_balance = self_balance;
                                bstate->s_time = j;
                                bstate->s_balance_pending_in = 0;
                                bhistory->s_history[j] = *bstate;
                                printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>5  %d: write history at %d\n", p_logic + 1, j);
                            }
                            cur_t = set_lamport_time(rec_header->s_local_time);
                        }
                        printf(">>>>>>>>>>>>>>>>>> %d recieve something length=%d  s_type=%d\n", p_logic + 1, result, rec_header->s_type);
                    }

                    if (result <= 0) continue;
                    if (result > 0 && rec_header->s_type == TRANSFER) {
                        TransferOrder* rec_tforder = (TransferOrder*)malloc(sizeof(TransferOrder));
                        result = read(pipeline[i][p_logic][0], (char*)rec_tforder, sizeof(TransferOrder));
                        int dst = rec_tforder->s_dst;
                        BalanceState* bstate = (BalanceState*)malloc(sizeof(BalanceState));
                        bstate->s_balance = self_balance;
                        bstate->s_time = cur_t;
                        bstate->s_balance_pending_in = rec_tforder->s_amount;
                        bhistory->s_history[cur_t] = *bstate;
                        cur_t = add_lamport_time();

                        //  log_transfer_in_fmt: "%d: process %1d received $%2d from process %1d\n";
                        fprintf(pFileEvent, log_transfer_in_fmt, p_logic + 1, p_logic + 1, self_balance, rec_tforder->s_src);
                        fprintf(pFilePipes, "%d: received \"TRANSFER\"", p_logic + 1);
                        printf(log_transfer_in_fmt, p_logic + 1, p_logic + 1, self_balance, rec_tforder->s_src);

                        if (dst - 1 == p_logic) {
                            MessageHeader* ack_header = (MessageHeader*)malloc(sizeof(MessageHeader));
                            ack_header->s_local_time = cur_t;
                            ack_header->s_type = ACK;
                            ack_header->s_magic = MESSAGE_MAGIC;
                            ack_header->s_payload_len = 0;
                            result = write(parent_pip[p_logic][1][1], (char*)ack_header, 8);
                            self_balance += rec_tforder->s_amount;
                            bstate = (BalanceState*)malloc(sizeof(BalanceState));
                            bstate->s_balance = self_balance;
                            bstate->s_time = cur_t;
                            bstate->s_balance_pending_in = 0;
                            bhistory->s_history[cur_t] = *bstate;
                        }
                    }// 目的进程
                    else if (rec_header->s_type == STARTED) {
                        BalanceState* bstate = (BalanceState*)malloc(sizeof(BalanceState));
                        bstate->s_balance = self_balance;
                        bstate->s_time = cur_t;
                        bstate->s_balance_pending_in = 0;
                        bhistory->s_history[cur_t] = *bstate;
                        char* buf = (char*)malloc(LENGTH);
                        result = read(pipeline[i][p_logic][0], (char*)buf, rec_header->s_payload_len);
                        /* TODO: nothing to do*/
                    }
                    else if (rec_header->s_type == DONE) {
                        BalanceState* bstate = (BalanceState*)malloc(sizeof(BalanceState));
                        bstate->s_balance = self_balance;
                        bstate->s_time = cur_t;
                        bstate->s_balance_pending_in = 0;
                        bhistory->s_history[cur_t] = *bstate;
                        char* buf = (char*)malloc(LENGTH);
                        result = read(pipeline[i][p_logic][0], (char*)buf, rec_header->s_payload_len);
                        printf("%d Recieve DONE from %d   length=%d\n", p_logic + 1, i + 1, result);
                        fprintf(pFileEvent, "%s\n", buf);
                        fprintf(pFilePipes, "%d Recieve DONE from %d   length=%d\n", p_logic + 1, i + 1, result);
                        is_done[i] = 1;
                    }
                }


            }
            else if (stage == 2) {
                // After receiving STOP message, child process should send DONE message to all processes
                printf("\n%d: Received \"STOP\" from parent process\n", p_logic + 1);
                fprintf(pFilePipes, "\n%d: Received \"STOP\" from parent process\n", p_logic + 1);

                char* buffer = (char*)malloc(LENGTH);
                char* buffer1 = (char*)malloc(LENGTH);
                memset(buffer, 0, LENGTH);
                int cur_t = add_lamport_time();
                MessageHeader* header = (MessageHeader*)malloc(sizeof(MessageHeader));
                header->s_local_time = cur_t;
                header->s_type = DONE;
                header->s_magic = MESSAGE_MAGIC;

                // log_done_fmt: "%d: process %1d has DONE with balance $%2d\n";
                fprintf(pFileEvent, log_done_fmt, p_logic + 1, p_logic + 1, self_balance);
                printf(log_done_fmt, p_logic + 1, p_logic + 1, self_balance);

                header->s_payload_len = strlen(buffer1);
                *((MessageHeader*)buffer) = *header;
                int len = strlen(buffer1);
                memcpy(buffer + 8, buffer1, strlen(buffer1));
                *(buffer + 8 + len + 1) = '\0';
                fprintf(pFilePipes, "%s", buffer1);
                for (i = 0; i < N; i++) {
                    if (i != p_logic) {
                        write(pipeline[p_logic][i][1], buffer, 8 + len);
                        printf("%d sent DONE to %d   length=%d\n", p_logic + 1, i + 1, len);
                    }
                }
                write(parent_pip[p_logic][1][1], buffer, 8 + len);

                BalanceState* bstate = (BalanceState*)malloc(sizeof(BalanceState));
                bstate->s_balance = self_balance;
                bstate->s_time = cur_t;
                bstate->s_balance_pending_in = 0;
                bhistory->s_history[cur_t] = *bstate;
                bhistory->s_history_len = cur_t + 1;
                stage = 3;
            }
            else if (stage == 3) {
                int last = 0;
                for (i = 0; i < N; i++) {
                    if (is_done[i] == 0) {
                        int result = -1;
                        MessageHeader* rec_done = (MessageHeader*)malloc(sizeof(MessageHeader));
                        result = read(pipeline[i][p_logic][0], (char*)rec_done, 8);
                        if (result > 1 && rec_done->s_type == DONE && i != p_logic) {
                            last++;
                            int cur_t = add_lamport_time();
                            if (cur_t <= rec_done->s_local_time)
                            {
                                for (int j = cur_t; j <= rec_done->s_local_time; j++)
                                {
                                    BalanceState* bstate = (BalanceState*)malloc(sizeof(BalanceState));
                                    bstate->s_balance = self_balance;
                                    bstate->s_time = j;
                                    bstate->s_balance_pending_in = 0;
                                    bhistory->s_history[j] = *bstate;
                                    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>%d: write history at %d\n", p_logic + 1, j);
                                }
                                cur_t = set_lamport_time(rec_done->s_local_time);
                            }
                            BalanceState* bstate = (BalanceState*)malloc(sizeof(BalanceState));
                            bstate->s_balance = self_balance;
                            bstate->s_time = cur_t;
                            bstate->s_balance_pending_in = 0;
                            bhistory->s_history[cur_t] = *bstate;
                            is_done[i] = 1;
                            char* buf = (char*)malloc(LENGTH);
                            result = read(pipeline[i][p_logic][0], buf, rec_done->s_payload_len);

                            printf("Child process-%d received \"DONE\", %d/%d\n", p_logic + 1, last, N - 1);
                            fprintf(pFilePipes, "Child process-%d received \"DONE\", %d/%d\n", p_logic + 1, last, N - 1);
                            fprintf(pFileEvent, "%s\n", buf);

                        }
                    }
                }
                if (last == 0) {
                    //  log_received_all_done_fmt: "%d: process %1d has DONE with balance $%2d\n";
                    fprintf(pFileEvent, log_received_all_done_fmt, p_logic + 1, p_logic + 1);
                    printf(log_received_all_done_fmt, p_logic + 1, p_logic + 1);
                    break;
                }
            }
            sleep(1);
        }
        cur_t = add_lamport_time();
        bstate = (BalanceState*)malloc(sizeof(BalanceState));
        bstate->s_balance = self_balance;
        bstate->s_time = cur_t;
        bstate->s_balance_pending_in = 0;
        bhistory->s_history[cur_t] = *bstate;
        MessageHeader* hist = (MessageHeader*)malloc(sizeof(MessageHeader));
        hist->s_local_time = cur_t;
        hist->s_type = BALANCE_HISTORY;
        hist->s_magic = MESSAGE_MAGIC;
        hist->s_payload_len = sizeof(BalanceHistory);
        char* h_buffer = (char*)malloc(LENGTH);
        *(MessageHeader*)h_buffer = *hist;
        bhistory->s_history_len = get_lamport_time();
        *(BalanceHistory*)(h_buffer + 8) = *bhistory;
        write(parent_pip[p_logic][1][1], h_buffer, 8 + sizeof(BalanceHistory));
        printf("Child process-%d sent history to parent process\n", p_logic + 1);
    }
    else {
        printf("\n----------Parent process \"START\" -----------------\n");
        fprintf(pFilePipes, "\n----------Parent process \"START\" -----------------\n");
        int* is_start = (int*)malloc(sizeof(int) * N);
        while (1) {  // waiting for "STARTED"
            int last = 0;

            for (i = 0; i < N; i++) {
                if (is_start[i] == 0) {
                    last++;
                    MessageHeader* rec_start = (MessageHeader*)malloc(sizeof(MessageHeader));
                    int result = -1;
                    result = read(parent_pip[i][1][0], (char*)rec_start, 8);
                    if (result > 1 && rec_start->s_type == STARTED) {

                        int cur_t = add_lamport_time();
                        if (cur_t <= rec_start->s_local_time)
                        {
                            cur_t = set_lamport_time(rec_start->s_local_time);
                        }
                        printf("Parent process recieve STARTED from %d\n", i + 1);
                        is_start[i] = 1;
                        char* buf = (char*)malloc(LENGTH);
                        result = read(parent_pip[i][1][0], (char*)rec_start, rec_start->s_payload_len);
                        fprintf(pFilePipes, "%s", buf);
                    }
                }
            }
            if (last == 0) {
                fprintf(pFileEvent, log_received_all_started_fmt, p_logic + 1, p_logic + 1);
                fprintf(pFilePipes, log_received_all_started_fmt, p_logic + 1, p_logic + 1);
                printf(log_received_all_started_fmt, p_logic + 1, p_logic + 1);
                break;
            }
        }

        bank_robbery(parent_pip, N);
        int cur_t = add_lamport_time();
        MessageHeader* rec_STOP = (MessageHeader*)malloc(sizeof(MessageHeader));
        rec_STOP->s_payload_len = 0;
        rec_STOP->s_local_time = cur_t;
        rec_STOP->s_type = STOP;
        rec_STOP->s_magic = MESSAGE_MAGIC;
        for (i = 0; i < N; i++) {
            write(parent_pip[i][0][1], (char*)rec_STOP, 8);
        }
        fprintf(pFilePipes, "Parent process sent \"STOP\"\n");
        int* all_done = (int*)malloc(sizeof(int) * N);
        while (1) {  // waiting for "DONE"
            int last = 0;
            // printf("%d\n",k);
            for (i = 0; i < N; i++) {
                if (all_done[i] == 0) {
                    last++;
                    MessageHeader* rec_done =
                        (MessageHeader*)malloc(sizeof(MessageHeader));
                    int result = -1;
                    result = read(parent_pip[i][1][0], (char*)rec_done, 8);
                    if (result > 1 && rec_done->s_type == DONE) {
                        cur_t = add_lamport_time();
                        if (cur_t <= rec_done->s_local_time)
                        {
                            cur_t = set_lamport_time(rec_done->s_local_time);
                        }
                        all_done[i] = 1;
                        char* buf = (char*)malloc(LENGTH);
                        result = read(parent_pip[i][1][0], buf, rec_done->s_payload_len);
                        fprintf(pFileEvent, "%s\n", buf);
                        printf("%s\n", buf);
                        fprintf(pFilePipes, "%s", buf);
                    }
                }
            }
            if (last == 0) {
                fprintf(pFileEvent, log_received_all_done_fmt, p_logic + 1, p_logic + 1);
                fprintf(pFilePipes, log_received_all_done_fmt, p_logic + 1, p_logic + 1);
                printf(log_received_all_done_fmt, p_logic + 1, p_logic + 1);
                break;
            }
        }
        AllHistory* allh = (AllHistory*)malloc(sizeof(AllHistory));
        allh->s_history_len = N;
        int* all_history = (int*)malloc(4 * N);
        while (1) {  // waiting for "History"
            int last = 0;
            for (i = 0; i < N; i++) {
                if (all_history[i] == 0) {
                    last++;
                    MessageHeader* rec_his = (MessageHeader*)malloc(sizeof(MessageHeader));
                    int result = -1;
                    result = read(parent_pip[i][1][0], (char*)rec_his, 8);
                    if (result > 1 && rec_his->s_type == BALANCE_HISTORY) {
                        printf("main process recieve history from %d\n", i + 1);
                        all_history[i] = 1;
                        BalanceHistory* s_his =
                            (BalanceHistory*)malloc(sizeof(BalanceHistory));
                        result = read(parent_pip[i][1][0], (char*)s_his, rec_his->s_payload_len);
                        allh->s_history[i] = *(s_his);
                    }
                }
            }
            if (last == 0) {
                printf("main process recieve all history\n");
                fprintf(pFilePipes, "main process recieve all history\n");
                break;
            }
        }

        print_history(allh);
    }

    if (fpid != 0) {
        int child;
        for (i = 0; i < N; i++) {
            wait(&child);
        }
    }
    return 0;
}

