# wbench  [![Build Status](http://www.web-lovers.com/assets/bimg/build_passing.png)](http://www.web-lovers.com/)

**wbench**, Dedicated to stress testing that supports ws services.

## Build

To build wbench from source [github]:

    $ git clone git@github.com:wettper/wbench.git
    $ cd wbench
    $ ./configure
    $ make
    $ sudo make install


A quick checklist:

+ Use newer version of gcc (older version of gcc has problems)

## Features

+ Support websocket stress test.

## Help

    Usage: wbench <options> <url>
    Options:
        -c, --connection    <N> Connections to keep open
        -t, --threads       <N> Number of threads to use

        -d, --data          <H> Add data to request
        -T, --timeout       <T> Socket/request timeout
        -v, --version       Print version details

    Numeric arguments may include a SI unit (1k, 1M, 1G)
    Time arguments may include a time unit (2s, 2m, 2h)


## Issues and Support

Have a bug or a question? Please create an issue here on GitHub!

https://github.com/wettper/wbench/issues

## Committers

* wettper ([@wettper](http://www.web-lovers.com))

Thank you to all of our [contributors](https://github.com/wettper/wbench/graphs/contributors)!

## License

Copyright 2017 wettper.

Licensed under the GNU General Public License v3.0
