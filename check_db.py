#!/usr/bin/env python2

# Checks the database dump created by dlldumper.exe against a JSON
# dump of the device database from the Linux software.

from __future__ import print_function

import json
import optparse
import sys

def main():

    op = optparse.OptionParser()
    op.add_option("--our-db", dest="our_db", help="JSON dump of the Linux minipro device database")
    op.add_option("--win-db", dest="win_db", help="JSON dump of InfoIC.dll device dataase (dlldumper.exe)")

    opts, args = op.parse_args()

    with open(opts.our_db) as f:
        our_db = dict((ic["name"], ic) for ic in json.load(f))

    with open(opts.win_db) as f:
        win_db = json.load(f)

    key_scores = {}

    mismatch_count = 0
    missing_count = 0

    for mfc in win_db:
        for win_ic in mfc["ics"]:
            name = win_ic["name"]
            our_ic = our_db.get(name)
            if our_ic:
                for k in win_ic.keys():
                    if k in our_ic:
                        correct, total = key_scores.get(k, (0, 0))
                        if win_ic[k] != our_ic[k]:
                            print("MISMATCH: in part <{}>, field {}: <{}> != <{}> (ours)".format(name, k, win_ic[k], our_ic[k]))
                            mismatch_count += 1
                        else:
                            correct += 1
                        key_scores[k] = (correct, total + 1)
            else:
                print("NOT FOUND: part {} was not found in our database".format(name))
                missing_count += 1
    for k, v in key_scores.iteritems():
        print("{}: {:.2f}% correct".format(k, (100.0 * v[0]) / v[1]))

    print("{} mismatches".format(mismatch_count))
    print("{} parts missing".format(missing_count))

if __name__ == "__main__":
    main()
