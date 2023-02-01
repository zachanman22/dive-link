import queue

l = [10,12,13,14]
q2 = queue.Queue()
q2.put(l)
q2.put(queue.deque(l))
while not q2.empty():
    print(q2.get())
