import threading
import time
import random
import argparse

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="DragonBoard Ping-Pong Simulation")
    parser.add_argument("--buffer", type=int, default=5, help="Size of the buffer")
    parser.add_argument("--items", type=int, default=10, help="Total items to process")
    args = parser.parse_args()

    #need to define head and tail pointers
    buffer = [None]*args.buffer
    prod_head = 0
    cons_tail = -1
    lock = threading.Lock()

    def producer():
        global prod_head, cons_tail

        item_id = 0
        while item_id < args.items:

            next_head = (prod_head + 1) % args.buffer
            if next_head != (cons_tail + 1) % args.buffer:  # buffer not full
                buffer[prod_head] = f"Item-{item_id}"
                print(f"Produced: {buffer[prod_head]} at position {prod_head}")
                prod_head = next_head
                item_id += 1
                time.sleep(0.1)

            time.sleep(0.2)
    def consumer():
        global prod_head, cons_tail

        consumed = 0
        while consumed < args.items:

            next_tail = (cons_tail + 1) % args.buffer
            if next_tail != prod_head:  # buffer not empty
                print("     Consumed:", buffer[next_tail], "from position", next_tail)
                cons_tail = next_tail
                consumed += 1
                time.sleep(0.15)

            time.sleep(0.2)

    prod_thread = threading.Thread(target=producer)
    cons_thread = threading.Thread(target=consumer)
    prod_thread.start()
    cons_thread.start()
    prod_thread.join()
    cons_thread.join()