/* p3helper.c
 * Matthew Hess
 * Program 3
 * CS570
 * Professor Carroll
 * SDSU
 * 11/2/18
 *
 * This is the only file you may change. (In fact, the other files should
 * be symbolic links to:
 *   ~cs570/Three/p3main.c
 *   ~cs570/Three/p3.h
 *   ~cs570/Three/Makefile
 *   ~cs570/Three/CHK.h    )
 *
 */
#include "p3.h"

/*
 * Abstraction:
 * Two type of threads will be calling functions in this p3helper.c file, passing their thread kind as a parameters when
 * invoking the functions.
 * These functions act as a toolset for solving an abstract problem of controlling different types (members) of threads (SHOOTERS
 * or JOGGERS) of a gym from entering the gym at one time. Multiple threads of the same type are allowed on the gym floor at one time,
 * but if at least one type of member is on the gym floor at one time the other type of threads (members) will have to wait in line until
 * the last thread (member) of the other type has left the gym. As long as at least one type is currently in gym then other
 * threads of the same type they will be granted access to the gym. This solution entails that the gym has hired a receptionist
 * that manages these threads abstractly. The SHOOTER members and the JOGGER members will have to enter the gym through the
 * lobby and must get into separate type lines before the receptionist check the members into the gym. If there is already a line
 * of members pertaining to that specific type then those threads (members) will have to take a seat in the lobby and wait along
 * with the other similar type of threads (members). We must clarify that members enter and exit through the lobby and that the
 * receptionist is always up to date with who and of what type is in the gym at all times because the gym doesn't have the
 * infrastructure to have both types of members on gym floor at one time.
 * Initially when the gym is empty and a particular type of thread enters the lobby and approaches the receptionist because
 * there is no line already of that threads particular type the receptionist will look at their log book. The receptionist
 * will see that the gym has no members of that threads type inside already and then will proceed to then check to see if the
 * other type is in the gym. If the other type of member is in the gym then the receptionist tells that thread to go wait
 * in its specific line of the threads type. If the other type is not in the gym then the receptionist will grant the
 * member of that specific type to be allowed into the gym blocking other members of different types from entering the gym.
 * If another thread of the same particular type comes in the lobby now he will see that there is no line formed so he will
 * proceed to talk to the receptionist. The receptionist will then check her ledger and see that there is already a member
 * of the same type in the gym at the present moment so that thread will be let in. Now if another similar type thread we're to
 * enter the lobby while the receptionist was being talked to by a similar type thread was talking to the receptionist then
 * that similar typed thread is instructed to sit and wait in the lobby in their specific line and once the receptionist
 * isn't talking to a similar type then that similar typed thread will be allowed to talk to the receptionist after. This
 * solution claims that the receptionist can talk to  * two separate types of threads(members) at once but only will
 * be allowed to talk to one thread (member) of that type of member at once. If a different type of member were to walk
 * in the lobby and precede to talk to the receptionist because there was no similar thread talking to the receptionist
 * at the present time and also there was no line already then that thread (member) would be granted to talk with the
 * receptionist and the receptionist would then tell that thread to wait because another type of member is already in the gym.
 * This will then make similar type threads to start a line.
 *
 * Because the receptionist can't leave their seat and because they need to be up to date with who is in the gym at all times the
 * and the fact that the receptionist can only talk with one thread of a particular type whether that thread is entering or
 * leaving at one time, the threads(members of a specific type) who are already in the gym must wait in line if there
 * is no similar thread already talking to the receptionist at the current time. If there isn't one then a thread(member)
 * can notify the receptionist that they are ready to leave preceding to leave the gym lobby to go home.
 *
 * This solution uses semaphores to put threads in line if there is already a line specific to that thread type making
 * that thread type wait patiently, and a semaphore that the receptionist controls if there is already a member of
 * a particular type in the gym. The allows the receptionist to keep only one type of member in the gym at one time.
 * If there is at least one person in the gym the receptionist locks the semaphore blocking the other type of members from entering
 * the gym causing a line to start for that particular other type. When the last person leaves the gym the semaphore will be
 * incremented unlocking the gym for any type to enter.
 */


/* Global Declarations */
int shooterCount, joggerCount = 0; // Counters for how many shooters or joggers are on the gym floor at one time
sem_t critical_region_gym; // prevents race conditions of two different types being on the gym floor the same time
sem_t shooterLock;// prevents race conditions of multiple shooters talking to the receptionist at one time
sem_t joggerLock; // prevents race conditions of multiple joggers talking to the receptionist at one time


void initstudentstuff(void) {
    // Initialization of semaphores
    CHK(sem_init(&critical_region_gym, LOCAL, 1)); // gym floor abstraction, only one kind (shooters or joggers) can be on the gym floor at one time
    CHK(sem_init(&shooterLock, LOCAL, 1)); // protects race conditions of multiple shooters talking to the receptionist at one time
    CHK(sem_init(&joggerLock, LOCAL, 1)); // protects race conditions of multiple joggers talking to the receptionist at one time
}

/* Note: we don't have to worry about more than MAXCUSTOMERS accessing the gym at one time because of how we create threads in p3main.c */

/*
 * Purpose: Get onto the gym floor, subject to the rules.
 */
void prolog(int kind) {
  if(kind == JOGGER){
    CHK(sem_wait(&joggerLock)); // if one jogger thread is already talking to the receptionist block the other same type thread callers
    joggerCount++;
    if (joggerCount == 1) CHK(sem_wait(&critical_region_gym)); // block this thread if the other type is currently in the gym, other wise block other type from the gym floor and let this thread enter the gym alongside its similar type of threads
    CHK(sem_post(&joggerLock)); // if the receptionist is done with talking to the jogger thread then allow another jogger to approach the receptionist
   }
   else if(kind == SHOOTER){
    CHK(sem_wait(&shooterLock)); // if one shooter thread is already talking to the receptionist block the other same type thread callers
    shooterCount++;
    if(shooterCount == 1) CHK(sem_wait(&critical_region_gym)); // block this thread if the other type is currently in the gym, other wise block other type from the gym floor and let this thread enter the gym alongside its similar type of threads
    CHK(sem_post(&shooterLock));  // if the receptionist is done with talking to the shooter thread then allow another shooter thread to approach the receptionist
   }
}

/*
 * Purpose: give up access permission to the gym floor, subject to the rules.
 */
void epilog(int kind) {
    if (kind == JOGGER) {
        CHK(sem_wait(&joggerLock)); // if one jogger thread is already talking to the receptionist block the other same type thread callers
        joggerCount--;
        if (joggerCount == 0) CHK(sem_post(&critical_region_gym)); // if this is the last jogger to leave the gym then the receptionist unlocks the gym for any other type to enter
        CHK(sem_post(&joggerLock)); // if the receptionist is done with talking to the jogger thread then allow another jogger thread to approach the receptionist
    } else if (kind == SHOOTER) {
        CHK(sem_wait(&shooterLock)); // if one shooter thread is already talking to the receptionist block the other same type thread callers
        shooterCount--;
        if (shooterCount == 0) CHK(sem_post(&critical_region_gym)); // if this is the last shooter to leave the gym then the receptionist unlocks the gym for any other type to enter
        CHK(sem_post(&shooterLock));  // if the receptionist is done with talking to the shooter thread then allow another shooter thread to approach the receptionist
    }
}