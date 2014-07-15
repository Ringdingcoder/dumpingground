# Run with Python 2.7

from __future__ import print_function

import time
import itertools
from multiprocessing import Pool, Process, Queue
import MySQLdb

def grouper(n, iterable, fillvalue=None):
    "grouper(3, 'ABCDEFG', 'x') --> ABC DEF Gxx"
    args = [iter(iterable)] * n
    return itertools.izip_longest(fillvalue=fillvalue, *args)

def get_cursor():
    conn = MySQLdb.connect(host = "192.168.10.65",
                           port = 3307,
                           user = "sr",
                           passwd = "thepassword",
                           db = "sr_test")
    return conn.cursor(), conn

def run(q):
    c, conn = get_cursor()
    # Works fine with these two settings combined
    #c.execute("set session optimizer_switch='use_index_extensions=off'")
    #c.execute("set session eq_range_index_dive_limit=500")

    while True:
        tgts = q.get()
        query = ("select * from EI where 0=1 or %s order by target, version, pos" %
                 " or ".join(["(target=%d and version=%d)" % cr for cr in tgts]))
        tstart = time.time()
        c.execute(query)
        l = len(c.fetchall())
        delay = time.time() - tstart
        slooow = delay >= .5
        print("%.4f seconds" % delay, l, "<-- !!" if slooow else "")
        if slooow:
            c.execute("explain " + query)
            print(c.fetchall())

def dotest():
    c, conn = get_cursor()

    NUM_PROCESSES = 8

    c.execute("select @@optimizer_switch")
    print(c.fetchall())

    c.execute("select distinct target, version from EI order by target, version")
    tgts = dict([(int(x[0]), int(x[1])) for x in c.fetchall()])
    tgts = sorted(tgts.iteritems())
    print(len(tgts), "distinct pairs")

    conn.close()

    q = Queue(NUM_PROCESSES*2)

    procs = [Process(target=run, args=(q,)) for n in range(NUM_PROCESSES)]
    for p in procs:
        p.start()

    for pairs in grouper(500, itertools.cycle(tgts)):
        pairs = list(pairs)
        q.put(pairs)

    for p in procs:
        p.join()

if __name__ == "__main__":
    dotest()
