#include <fcntl.h>
#include <iostream>
#include <semaphore.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ScreenReader.h"
#include "ScreenProcessor.h"
#include "LedProcessor.h"

#define NUM_LEDS_WIDTH 32
#define NUM_LEDS_HEIGHT 20
#define BPP 4
#define LED_PIXELS_COMPENSATION 15

const int NUM_LEDS_TOTAL = 2 * (NUM_LEDS_WIDTH + NUM_LEDS_HEIGHT);
const long MILLIS_TO_NANOS = 1000000;

const char SHM_NAME[] = "/shm_leds";
const char SEM_NAME[] = "/sem_leds";
const mode_t SHM_MODE = 0777;
const off_t SHM_SIZE = NUM_LEDS_TOTAL*3;
int shm_fd;
void *shm = NULL;
sem_t *sem = NULL;

int createAndOpenShm(){
    // Shared memory
    shm_fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, SHM_MODE);
    if(shm_fd == -1) return 1;

    if(ftruncate(shm_fd, SHM_SIZE) < 0) return 1;

    shm = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(shm == MAP_FAILED) return 1;

    // Semaphore
    if (sem_unlink(SEM_NAME) != 0){
        fprintf(stderr,
            "createAndOpenShm: could not unlink old semaphore; "
            "this is normal as previous executions of this program are "
            "supposed to cleanup before exiting\n"
        );
    }

    sem = sem_open(SEM_NAME, O_CREAT, 0777, 1);

    if (sem == SEM_FAILED){
        return 1;
    }

    return 0;
}

int closeAndDeleteShm(){
    if(munmap((void*)shm, SHM_SIZE) != 0) return 1;

    if(close(shm_fd) != 0) return 1;

    if(shm_unlink(SHM_NAME) != 0) return 1;

    if (sem_destroy(sem) != 0) return 1;

    if (sem_unlink(SEM_NAME) != 0) return 1;

    return 0;
}

// Flatten array of LED color components
// and write to shared memory
int writeToShm(LedProcessor &ledProcessor){
    sem_wait(sem);

    uint8_t *shm_leds = (uint8_t*)shm;
    ledProcessor.copy(shm_leds);

    sem_post(sem);
    return 0;
}

void ledPrint(){
    uint8_t *leds = (uint8_t*)shm;

    int i = 0;
    int range = NUM_LEDS_WIDTH;
    printf("Bottom: ");
    for (; i < range; i++) printf("%02x%02x%02x ", leds[3*i], leds[3*i+1], leds[3*i+2]);
    printf("\n");

    printf("Left   :");
    range += NUM_LEDS_HEIGHT;
    for (; i < range; i++) printf("%02x%02x%02x ", leds[3*i], leds[3*i+1], leds[3*i+2]);
    printf("\n");

    printf("Top    :");
    range += NUM_LEDS_WIDTH;
    for (; i < range; i++) printf("%02x%02x%02x ", leds[3*i], leds[3*i+1], leds[3*i+2]);
    printf("\n");

    printf("Right  :");
    range += NUM_LEDS_HEIGHT;
    for (; i < range; i++) printf("%02x%02x%02x ", leds[3*i], leds[3*i+1], leds[3*i+2]);
    printf("\n\n");
}

void lock_forever(){
    sem_t sem;
    sem_init(&sem, 0, 0);
    sem_wait(&sem);
}

enum AlarmTask : int {
    UPDATE_SHM
};

LedProcessor *ledProcessor = nullptr;

void callbackSIGINT(int sig, siginfo_t *info, void *ucontext) {
    fprintf(stderr, "Signal callback\n");

    if(closeAndDeleteShm()) exit(1);

    delete ledProcessor;

    exit(0);
}

int setupSIGINT(){
    struct sigaction act;
    act.sa_flags = SA_SIGINFO | SA_RESTART;
    act.sa_sigaction = callbackSIGINT;

    if(sigaction(SIGINT, &act, NULL) != 0){ perror("sigaction int"); return 1; }

    return 0;
}

void updateShm(){
    ledProcessor->update();

    if (writeToShm(*ledProcessor) == 0){
        ledPrint();
    } else {
        printf("Error writting to shared memory");
    }
}

void callbackSIGALRM(int sig, siginfo_t *info, void *ucontext){
    switch(info->si_value.sival_int){
        case UPDATE_SHM:
            updateShm();
            break;
        default:
            fprintf(stderr, "callbackSIGALRM: got unknown task %d\n", info->si_value.sival_int);
            break;
    }
}

int setupSIGALRM(){
    struct sigaction act;
    act.sa_flags = SA_SIGINFO | SA_RESTART;
    act.sa_sigaction = callbackSIGALRM;
    
    if(sigaction(SIGALRM, &act, NULL) != 0){ perror("sigaction alrm"); return 1; }

    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGALRM;
    sev.sigev_value.sival_int = UPDATE_SHM;

    timer_t timerid;
    if(timer_create(CLOCK_REALTIME, &sev, &timerid) != 0){ perror("timer_create"); return 1; }
    
    struct itimerspec ts = {
        { 0, 50*MILLIS_TO_NANOS },
        { 0, 50*MILLIS_TO_NANOS }
    };

    if(timer_settime(timerid, 0, &ts, NULL) != 0){ perror("timer_settime"); return 1; }

    return 0;
}

int main()
{
    if(createAndOpenShm()) {
        fprintf(stderr, "[SCREENREADER] Could not open shared memory");
        return 1;
    }

    if(setupSIGINT()){
        fprintf(stderr, "[SCREENREADER] Could not setup SIGINT");
        return 1;
    }

    if(setupSIGALRM()){
        fprintf(stderr, "[SCREENREADER] Could not setup SIGALRM");
        return 1;
    }

    ScreenReader screen;

    const int PIXELS_PER_LED_AVG_X = screen.getScreenWidth () / NUM_LEDS_WIDTH;
    const int PIXELS_PER_LED_AVG_Y = screen.getScreenHeight() / NUM_LEDS_HEIGHT;
    ScreenProcessor screenProcessor(
        screen,
        PIXELS_PER_LED_AVG_X,
        PIXELS_PER_LED_AVG_Y
    );

    const float SCALE_FACTOR = 0.05;
    ledProcessor = new LedProcessor(screenProcessor, NUM_LEDS_WIDTH, NUM_LEDS_HEIGHT, SCALE_FACTOR);

    lock_forever();

    return 0;
}
