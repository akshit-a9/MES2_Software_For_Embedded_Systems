import threading
import time

varA = 0
varB = 0

mutex_A = threading.Lock()
mutex_B = threading.Lock()


def taskA():
    print("[taskA] Trying to acquire mutex_A...")
    with mutex_A:
        print("[taskA] Acquired mutex_A. Modifying varA...")
        varA_local = varA + 10

        time.sleep(1)

        print("[taskA] Trying to acquire mutex_B...")
        with mutex_B:                      
            print("[taskA] Acquired mutex_B. Modifying varB...")
            varB_local = varA_local + varB + 20
            print(f"[taskA] Done. varA={varA_local}, varB={varB_local}")


def taskB():
    print("[taskB] Trying to acquire mutex_B...")
    with mutex_B:
        print("[taskB] Acquired mutex_B. Modifying varB...")
        varB_local = varB + 20
        time.sleep(1)

        print("[taskB] Trying to acquire mutex_A...")
        with mutex_A:                      
            print("[taskB] Acquired mutex_A. Modifying varA...")
            varA_local = varB_local + varA + 10
            print(f"[taskB] Done. varA={varA_local}, varB={varB_local}")


if __name__ == "__main__":

    t1 = threading.Thread(target=taskA, name="taskA")
    t2 = threading.Thread(target=taskB, name="taskB")

    t1.start()
    t2.start()

    timeout = 5
    t1.join(timeout=timeout)
    t2.join(timeout=timeout)

    if t1.is_alive() or t2.is_alive():
        print(f"taskA alive: {t1.is_alive()}")
        print(f"taskB alive: {t2.is_alive()}")
        print("Both threads are stuck waiting for each other's mutex.")
    else:
        print("\n[MAIN] Both tasks completed (no deadlock occurred).")
