#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_PROCESSES 64
#define MAX_CHILDREN 64
#define MAX_LINE_LEN 256
#define SNAPSHOT_BUF_SIZE 8192

/*
 * LEGAL STATE TRANSITIONS:
 * - NULL -> RUNNING: A process is created via pm_fork() as RUNNING.
 * - RUNNING -> ZOMBIE: A process is terminated by pm_exit() or pm_kill().
 * - RUNNING -> BLOCKED: A parent calls pm_wait() but no children have exited yet.
 * - BLOCKED -> RUNNING: A parent wakes up from pm_wait() when a child exits.
 * - BLOCKED -> ZOMBIE: A parent currently blocked in wait is terminated via pm_kill().
 * - ZOMBIE -> TERMINATED (reaped): A parent successfully retrieves the zombie's exit status.
 */

typedef enum {
    STATE_RUNNING = 0,
    STATE_BLOCKED = 1,
    STATE_ZOMBIE = 2,
    STATE_TERMINATED = 3
} ProcessState;

typedef struct {
    int pid;
    int ppid;
    ProcessState state;
    int exit_status;
    int children[MAX_CHILDREN];
    int child_count;
    int used;
    pthread_cond_t wait_cond;
} PCB;

typedef struct SnapshotNode {
    char *text;
    struct SnapshotNode *next;
} SnapshotNode;

typedef struct {
    int thread_id;
    const char *filename;
} WorkerArg;

static PCB process_table[MAX_PROCESSES];
static int next_pid = 2;

/* Returns the number of active (used) slots — replaces the old table_size counter.
 * Counting directly from the used flags is the single source of truth and can
 * never drift out of sync with the actual table contents.
 * NOTE on orphan processes: this simulator does NOT re-parent orphans to PID 1
 * when their parent exits.  In a real OS, pm_exit would walk every PCB and set
 * ppid = 1 for any child whose parent is the dying process, then wake PID 1 if
 * it is blocked in wait.  That is a deliberate simplification here. */
__attribute__((unused)) static int count_used(void) {
    int n = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (process_table[i].used) n++;
    return n;
}

static pthread_mutex_t table_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t monitor_cond = PTHREAD_COND_INITIALIZER;

static SnapshotNode *snapshot_head = NULL;
static SnapshotNode *snapshot_tail = NULL;
static int all_done = 0;

static FILE *snap_file = NULL;

static const char *state_name(ProcessState s) {
    switch (s) {
        case STATE_RUNNING:    return "RUNNING";
        case STATE_BLOCKED:    return "BLOCKED";
        case STATE_ZOMBIE:     return "ZOMBIE";
        case STATE_TERMINATED: return "TERMINATED";
        default:               return "UNKNOWN";
    }
}

static PCB *find_pcb(int pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].used && process_table[i].pid == pid)
            return &process_table[i];
    }
    return NULL;
}

static int find_free_slot(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!process_table[i].used)
            return i;
    }
    return -1;
}

static void add_child(PCB *parent, int child_pid) {
    if (parent != NULL && parent->child_count < MAX_CHILDREN)
        parent->children[parent->child_count++] = child_pid;
}

static void remove_child(PCB *parent, int child_pid) {
    if (parent == NULL) return;
    for (int i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == child_pid) {
            for (int j = i; j < parent->child_count - 1; j++)   /*left shift*/
                parent->children[j] = parent->children[j + 1];
            parent->child_count--;
            return;
        }
    }
}

static int is_real_child(PCB *parent, int child_pid) {
    if (parent == NULL) return 0;
    for (int i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == child_pid) return 1;
    }
    return 0;
}

static PCB *find_zombie_child(PCB *parent, int child_pid) {
    if (parent == NULL) return NULL;
    if (child_pid == -1) {
        for (int i = 0; i < parent->child_count; i++) {
            PCB *child = find_pcb(parent->children[i]);
            if (child != NULL && child->state == STATE_ZOMBIE)
                return child;
        }
        return NULL;
    }
    PCB *child = find_pcb(child_pid);
    if (child != NULL && child->state == STATE_ZOMBIE && child->ppid == parent->pid)
        return child;
    return NULL;
}

static char *build_snapshot(const char *label) {
    char *buf = (char *)malloc(SNAPSHOT_BUF_SIZE);         /*dynamic allocation*/
    if (buf == NULL) return NULL;

    size_t used = 0;
    int written = snprintf(buf + used, SNAPSHOT_BUF_SIZE - used,                     /*prevent buffer overflow*/
                           "%s\n%-8s %-8s %-12s %s\n----------------------------------------------\n",
                           label, "PID", "PPID", "STATE", "EXIT_STATUS");
    if (written < 0) { free(buf); return NULL; }
    used += (size_t)written;

    for (int i = 0; i < MAX_PROCESSES && used < SNAPSHOT_BUF_SIZE; i++) {
        PCB *p = &process_table[i];
        if (!p->used || p->state == STATE_TERMINATED) continue;

        if (p->state == STATE_ZOMBIE) {
            written = snprintf(buf + used, SNAPSHOT_BUF_SIZE - used,
                               "%-8d %-8d %-12s %d\n",
                               p->pid, p->ppid, state_name(p->state), p->exit_status);
        } else {
            written = snprintf(buf + used, SNAPSHOT_BUF_SIZE - used,
                               "%-8d %-8d %-12s -\n",
                               p->pid, p->ppid, state_name(p->state));
        }
        if (written < 0) { free(buf); return NULL; }
        used += (size_t)written;
    }

    if (used < SNAPSHOT_BUF_SIZE - 2) {
        buf[used++] = '\n';
        buf[used]   = '\0';
    } else {
        buf[SNAPSHOT_BUF_SIZE - 2] = '\n';
        buf[SNAPSHOT_BUF_SIZE - 1] = '\0';
    }
    return buf;
}

static void enqueue_snapshot(const char *label) {
    char *text = build_snapshot(label);
    if (text == NULL) return;

    SnapshotNode *node = (SnapshotNode *)malloc(sizeof(SnapshotNode));
    if (node == NULL) { free(text); return; }

    node->text = text;
    node->next = NULL;

    if (snapshot_tail == NULL) {
        snapshot_head = snapshot_tail = node;
    } else {
        snapshot_tail->next = node;
        snapshot_tail = node;
    }
    pthread_cond_signal(&monitor_cond);
}

/* ---------- public API ---------- */

static void pm_ps(void) {           /*print current process table */
    pthread_mutex_lock(&table_mutex);
    char *snapshot = build_snapshot("");
    pthread_mutex_unlock(&table_mutex);
    if (snapshot != NULL) {
        printf("%s", snapshot);
        free(snapshot);
    }
}

static int pm_fork(int parent_pid, int thread_id) {
    pthread_mutex_lock(&table_mutex);

    PCB *parent = find_pcb(parent_pid);
    if (parent == NULL) {
        fprintf(stderr, "[Thread %d] pm_fork: parent PID %d not found\n", thread_id, parent_pid);
        pthread_mutex_unlock(&table_mutex);
        return -1;
    }
    if (parent->state != STATE_RUNNING) {
        fprintf(stderr, "[Thread %d] pm_fork: parent PID %d is not RUNNING\n", thread_id, parent_pid);
        pthread_mutex_unlock(&table_mutex);
        return -1;
    }
    /* Use find_free_slot as the single capacity check: if it returns -1
     * there are no free slots, which is equivalent to the table being full. */
    int slot = find_free_slot();
    if (slot == -1) {
        fprintf(stderr, "[Thread %d] pm_fork: process table full\n", thread_id);
        pthread_mutex_unlock(&table_mutex);
        return -1;
    }

    int new_pid = next_pid++;
    PCB *child = &process_table[slot];
    child->pid         = new_pid;
    child->ppid        = parent_pid;
    child->state       = STATE_RUNNING;
    child->exit_status = 0;
    child->child_count = 0;
    child->used        = 1;
    pthread_cond_init(&child->wait_cond, NULL);

    add_child(parent, new_pid);
    /* No table_size counter — slot.used = 1 is the record of occupancy. */

    char label[128];
    snprintf(label, sizeof(label), "Thread %d calls pm_fork %d", thread_id, parent_pid);
    enqueue_snapshot(label);

    pthread_mutex_unlock(&table_mutex);
    return new_pid;
}

static void pm_exit(int pid, int status, int thread_id) {
    pthread_mutex_lock(&table_mutex);

    PCB *proc = find_pcb(pid);
    if (proc == NULL) {
        fprintf(stderr, "[Thread %d] pm_exit: PID %d not found\n", thread_id, pid);
        pthread_mutex_unlock(&table_mutex);
        return;
    }
    if (proc->state == STATE_ZOMBIE || proc->state == STATE_TERMINATED) {
        fprintf(stderr, "[Thread %d] pm_exit: PID %d already exited\n", thread_id, pid);
        pthread_mutex_unlock(&table_mutex);
        return;
    }

    proc->exit_status = status;
    proc->state= STATE_ZOMBIE;

    /* Wake any parent that is blocked waiting for this process. */
    PCB *parent = find_pcb(proc->ppid);
    if (parent != NULL)
        pthread_cond_broadcast(&parent->wait_cond);
        
    /* Wake the process itself in case a thread is blocked in pm_wait on its behalf */
    pthread_cond_broadcast(&proc->wait_cond);

    char label[128];
    snprintf(label, sizeof(label), "Thread %d calls pm_exit %d %d", thread_id, pid, status);
    enqueue_snapshot(label);

    pthread_mutex_unlock(&table_mutex);
}

static void pm_kill(int pid, int thread_id) {
    pthread_mutex_lock(&table_mutex);

    PCB *proc = find_pcb(pid);
    if (proc == NULL) {
        fprintf(stderr, "[Thread %d] pm_kill: PID %d not found\n", thread_id, pid);
        pthread_mutex_unlock(&table_mutex);
        return;
    }
    if (proc->state == STATE_ZOMBIE || proc->state == STATE_TERMINATED) {
        fprintf(stderr, "[Thread %d] pm_kill: PID %d already dead\n", thread_id, pid);
        pthread_mutex_unlock(&table_mutex);
        return;
    }

    proc->exit_status = -1;
    proc->state= STATE_ZOMBIE;

    /* Wake any parent blocked in wait. */
    PCB *parent = find_pcb(proc->ppid);
    if (parent != NULL)
        pthread_cond_broadcast(&parent->wait_cond);
        
    /* Wake the process itself in case a thread is blocked in pm_wait on its behalf */
    pthread_cond_broadcast(&proc->wait_cond);

    char label[128];
    snprintf(label, sizeof(label), "Thread %d calls pm_kill %d", thread_id, pid);
    enqueue_snapshot(label);

    pthread_mutex_unlock(&table_mutex);
}

/*
 * pm_wait:
 *   - Returns the exit status of the reaped child on success.
 *   - Returns -1 if parent has no children, child_pid is not a real child,
 *     or the parent itself died while waiting.
 *   - Blocks (STATE_BLOCKED) until a matching zombie child appears.
 *   - child_pid == -1: wait for any child.
 *
 */
static int pm_wait(int parent_pid, int child_pid, int thread_id) {
    pthread_mutex_lock(&table_mutex);

    PCB *parent = find_pcb(parent_pid);
    if (parent == NULL) {
        fprintf(stderr, "[Thread %d] pm_wait: parent PID %d not found\n", thread_id, parent_pid);
        pthread_mutex_unlock(&table_mutex);
        return -1;
    }
    if (parent->state == STATE_ZOMBIE || parent->state == STATE_TERMINATED) {
        fprintf(stderr, "[Thread %d] pm_wait: parent PID %d is not alive\n", thread_id, parent_pid);
        pthread_mutex_unlock(&table_mutex);
        return -1;
    }
    if (parent->child_count == 0) {
        /* Trivial return: no children to wait for. */
        pthread_mutex_unlock(&table_mutex);
        return -1;
    }
    if (child_pid != -1 && !is_real_child(parent, child_pid)) {
        fprintf(stderr, "[Thread %d] pm_wait: PID %d is not a child of %d\n",
                thread_id, child_pid, parent_pid);
        pthread_mutex_unlock(&table_mutex);
        return -1;
    }

    while (1) {
        /* Check if parent itself was killed while we slept. */
        if (parent->state == STATE_ZOMBIE || parent->state == STATE_TERMINATED) {
            pthread_mutex_unlock(&table_mutex);
            return -1;
        }

        /* If waiting for any child and none remain, stop. */
        if (child_pid == -1 && parent->child_count == 0) {
            parent->state = STATE_RUNNING;  /* unconditional — always correct final state */
            pthread_mutex_unlock(&table_mutex);
            return -1;
        }

        /* If waiting for a specific child and it was already reaped by someone else. */
        if (child_pid != -1 && !is_real_child(parent, child_pid)) {
            parent->state = STATE_RUNNING;  /* unconditional — always correct final state */
            pthread_mutex_unlock(&table_mutex);
            return -1;
        }

        PCB *zombie = find_zombie_child(parent, child_pid);
        if (zombie != NULL) {
            int status    = zombie->exit_status;
            int reaped_pid = zombie->pid;

            /* Unconditionally restore parent to RUNNING before the snapshot,
             * regardless of whether it was BLOCKED or RUNNING when we got here.
             * This is the correct final state and ensures the snapshot always
             * shows RUNNING, never a stale BLOCKED. */
            parent->state = STATE_RUNNING;

            /* Reap the zombie: remove from parent's child list and free the slot. */
            remove_child(parent, reaped_pid);
            pthread_cond_destroy(&zombie->wait_cond);
            memset(zombie, 0, sizeof(PCB));  /* used=0 clears the slot; all fields zeroed */
            /* No table_size counter to decrement — used=0 is the record of vacancy. */

            char label[128];
            snprintf(label, sizeof(label), "Thread %d calls pm_wait %d %d",
                     thread_id, parent_pid, child_pid);
            enqueue_snapshot(label);

            pthread_mutex_unlock(&table_mutex);
            return status;
        }

        /* No zombie yet — block the parent. */
        parent->state = STATE_BLOCKED;
        pthread_cond_wait(&parent->wait_cond, &table_mutex);
        /* Loop again: re-check conditions after waking. */
    }
}

/* ---------- script interpreter ---------- */

static void execute_script(const char *filename, int thread_id) {
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "[Thread %d] Could not open script: %s\n", thread_id, filename);
        return;
    }

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), f) != NULL) {
        line[strcspn(line, "\n")] = '\0';
        if (line[0] == '\0' || line[0] == '#') continue;

        char cmd[64];
        if (sscanf(line, "%63s", cmd) != 1) continue;

        if (strcmp(cmd, "fork") == 0) {
            int parent_pid;
            if (sscanf(line, "%*s %d", &parent_pid) == 1)
                pm_fork(parent_pid, thread_id);
        } else if (strcmp(cmd, "exit") == 0) {
            int pid, status;
            if (sscanf(line, "%*s %d %d", &pid, &status) == 2)   /*read 2nd, 3rd value*/
                pm_exit(pid, status, thread_id);
        } else if (strcmp(cmd, "wait") == 0) {
            int parent_pid, child_pid_arg;
            if (sscanf(line, "%*s %d %d", &parent_pid, &child_pid_arg) == 2) {
                int status = pm_wait(parent_pid, child_pid_arg, thread_id);
            }
        } else if (strcmp(cmd, "kill") == 0) {
            int pid;
            if (sscanf(line, "%*s %d", &pid) == 1)
                pm_kill(pid, thread_id);
        } else if (strcmp(cmd, "sleep") == 0) {
            int ms;
            if (sscanf(line, "%*s %d", &ms) == 1)
                usleep((useconds_t)ms * 1000);
        } else if (strcmp(cmd, "ps") == 0) {
            pm_ps();
        } else {
            fprintf(stderr, "[Thread %d] Unknown command: %s\n", thread_id, cmd);
        }
    }
    fclose(f);
}

/* ---------- thread entry points ---------- */

static void *worker_thread(void *arg) {
    WorkerArg *wa = (WorkerArg *)arg;
    execute_script(wa->filename, wa->thread_id);
    return NULL;
}

static void *monitor_thread(void *arg) {
    (void)arg;

    pthread_mutex_lock(&table_mutex);
    while (1) {
        /* Sleep until there is something to process or we are told to quit. */
        while (snapshot_head == NULL && !all_done)
            pthread_cond_wait(&monitor_cond, &table_mutex);

        if (snapshot_head == NULL && all_done)
            break;

        /* Drain all pending snapshots while holding the lock just long enough
           to detach the node, then release for I/O. */
        SnapshotNode *node = snapshot_head;
        snapshot_head = snapshot_head->next;
        if (snapshot_head == NULL)
            snapshot_tail = NULL;

        pthread_mutex_unlock(&table_mutex);

        if (snap_file != NULL) {
            fputs(node->text, snap_file);
            fflush(snap_file);
        }
        printf("%s", node->text);
        free(node->text);
        free(node);

        pthread_mutex_lock(&table_mutex);
    }
    pthread_mutex_unlock(&table_mutex);
    return NULL;
}

/* ---------- lifecycle ---------- */

static void init_process_manager(void) {
    memset(process_table, 0, sizeof(process_table));
    next_pid       = 2;
    snapshot_head  = snapshot_tail = NULL;
    all_done       = 0;

    /* Create the init process (PID 1, PPID 0). */
    PCB *init         = &process_table[0];
    init->pid         = 1;
    init->ppid        = 0;
    init->state       = STATE_RUNNING;
    init->exit_status = 0;
    init->child_count = 0;
    init->used        = 1;  /* marks this slot occupied; no separate counter needed */
    pthread_cond_init(&init->wait_cond, NULL);

    snap_file = fopen("snapshots.txt", "w");
    if (snap_file == NULL)
        fprintf(stderr, "Warning: could not open snapshots.txt for writing\n");

    /* Emit the initial snapshot. */
    pthread_mutex_lock(&table_mutex);
    enqueue_snapshot("Initial Process Table");
    pthread_mutex_unlock(&table_mutex);
}

static void cleanup_process_manager(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].used)
            pthread_cond_destroy(&process_table[i].wait_cond);
    }

    while (snapshot_head != NULL) {
        SnapshotNode *tmp = snapshot_head;
        snapshot_head = snapshot_head->next;
        free(tmp->text);
        free(tmp);
    }
    snapshot_tail = NULL;

    if (snap_file != NULL) {
        fclose(snap_file);
        snap_file = NULL;
    }

    pthread_mutex_destroy(&table_mutex);
    pthread_cond_destroy(&monitor_cond);
}

/* ---------- main ---------- */

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s thread0.txt thread1.txt ...\n", argv[0]);
        return 1;
    }

    int num_workers = argc - 1;

    init_process_manager();

    pthread_t monitor_tid;
    if (pthread_create(&monitor_tid, NULL, monitor_thread, NULL) != 0) {
        fprintf(stderr, "Failed to create monitor thread\n");
        cleanup_process_manager();
        return 1;
    }

    pthread_t *workers      = (pthread_t *)malloc(sizeof(pthread_t) * (size_t)num_workers);
    WorkerArg *worker_args  = (WorkerArg *)malloc(sizeof(WorkerArg) * (size_t)num_workers);
    /*
     * FIX: Use a separate validity array instead of relying on pthread_t == 0,
     * since pthread_t is an opaque type (may be a struct on some platforms).
     */
    int *worker_valid = (int *)calloc((size_t)num_workers, sizeof(int));

    if (workers == NULL || worker_args == NULL || worker_valid == NULL) {
        fprintf(stderr, "Memory allocation failed for worker threads\n");
        free(workers);
        free(worker_args);
        free(worker_valid);

        pthread_mutex_lock(&table_mutex);
        all_done = 1;
        pthread_cond_signal(&monitor_cond);
        pthread_mutex_unlock(&table_mutex);

        pthread_join(monitor_tid, NULL);
        cleanup_process_manager();
        return 1;
    }

    for (int i = 0; i < num_workers; i++) {
        worker_args[i].thread_id = i;
        worker_args[i].filename  = argv[i + 1];
        if (pthread_create(&workers[i], NULL, worker_thread, &worker_args[i]) != 0) {
            fprintf(stderr, "Failed to create worker thread %d\n", i);
            worker_valid[i] = 0;
        } else {
            worker_valid[i] = 1;
        }
    }

    for (int i = 0; i < num_workers; i++) {
        if (worker_valid[i])
            pthread_join(workers[i], NULL);
    }

    pthread_mutex_lock(&table_mutex);
    all_done = 1;
    pthread_cond_signal(&monitor_cond);
    pthread_mutex_unlock(&table_mutex);

    pthread_join(monitor_tid, NULL);

    free(workers);
    free(worker_args);
    free(worker_valid);

    cleanup_process_manager();
    return 0;
}
