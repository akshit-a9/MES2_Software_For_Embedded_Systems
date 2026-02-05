import threading
import time
import random
import argparse

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="DragonBoard Ping-Pong Simulation")
    parser.add_argument("--buffer", type=int, default=5, help="Size of the buffer")
    parser.add_argument("--items", type=int, default=10, help="Total items to process")
    args = parser.parse_args()

    BUFFER_SIZE = args.buffer
    # Two buffers: ping (0) and pong (1)
    buffer = [[None]*BUFFER_SIZE, [None]*BUFFER_SIZE]
    write_buf = 0
    read_buf  = 1
    data_ready = [False, False]
    lock = threading.Lock()

    def producer():
        global write_buf, read_buf

        item_id = 0
        while item_id < args.items:
            with lock:
                if not data_ready[write_buf]:
                    print(f"Producer writing to buffer {write_buf}")

                    for i in range(BUFFER_SIZE):
                        if item_id >= args.items:
                            break
                        buffer[write_buf][i] = f"item-{item_id}"
                        print(f"   produced: item-{item_id}")
                        item_id += 1
                        time.sleep(0.1)

                    data_ready[write_buf] = True

                    # swap buffers
                    write_buf, read_buf = read_buf, write_buf

            time.sleep(0.2)


    def consumer():
        global write_buf, read_buf

        consumed = 0
        while consumed < args.items:
            with lock:
                if data_ready[read_buf]:
                    print(f"Consumer reading from buffer {read_buf}")

                    for item in buffer[read_buf]:
                        if item is None:
                            continue
                        print("   consumed:", item)
                        consumed += 1
                        time.sleep(0.15)

                    data_ready[read_buf] = False

            time.sleep(0.2)


    producer_thread = threading.Thread(target=producer)
    consumer_thread = threading.Thread(target=consumer)

    producer_thread.start()
    consumer_thread.start()

    producer_thread.join()
    consumer_thread.join()
    print("Done")