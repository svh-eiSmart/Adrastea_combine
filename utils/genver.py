#!/usr/bin/env python

import sys
import os
import getopt
import re
import argparse

def main(opts):
    try:
        version_tags = opts.inverpath + "version"
        buildType = os.environ.get('buildType')

        with open(version_tags, 'r') as f:
            content = f.read()
            refsw = re.search(r'REFSW\s+(\w+)', content).group(1)
            major = int(re.search(r'MAJOR\s+(\d+)', content).group(1))
            minor = int(re.search(r'MINOR\s+(\d+)', content).group(1))
            codeline = int(re.search(r'CODELINE\s+(\d+)', content).group(1))
            project = int(re.search(r'PROJECT\s+(\d+)', content).group(1))
            match = re.search(r'(BUILD\s+)(\d+)', content)
            if str(os.getenv("Nightly")).lower() == "true":
                project = 90

            build = int(match.group(2))
            verstr = "{:s}_{:02d}_{:02d}_{:02d}_{:02d}_{:05d}".format(refsw, major, minor, codeline, project, build)

            if buildType != "RELEASE":
                verstr += "_" + opts.postfix

            print verstr

    except IOError as err:
        sys.exit(err)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Release tool.")
    parser.add_argument('--postfix', action='store', default='LO',
                        help='specify the version of the package')
    parser.add_argument('--inverpath', action='store', default='./',
                        help='specify the input path of version file')
    main(parser.parse_args())
