# Gumbo PHP

**Gumbo PHP** is low-level extension for HTML5 parsing.

[![Software License][ico-license]](LICENSE.md)
[![Build Status][ico-travis]][link-travis]
[![PHP 7 ready][ico-php7ready]][link-travis]

**Gumbo PHP** builds [DOMDocument](http://php.net/manual/en/class.domdocument.php) using [Gumbo HTML5 Parser](https://github.com/google/gumbo-parser). This solution solves all problems with HTML5 parsing or pages with inline JavaScript.

```php
use Layershifter\Gumbo\Parser;

$document = Parser::load('<a>Apples and bananas.</a>');
var_dump($document->saveHTML());

string(33) "<a>Apples and bananas.</a>
"
```

## Requirements

The following versions of PHP are supported.

* PHP 5.6
* PHP 7.0

## Install

To build `gumbo-php` extenstion PHP-devel package is required. The package should contain phpize utility.

```bash
$ git clone https://github.com/layershifter/gumbo-php.git
$ cd gumbo-php
$ phpize
$ ./configure
$ make
$ make install
```

This will build a 'gumbo.so' shared extension, load it in php.ini using:

```ini
[gumbo]
extension = gumbo.so
```

## Testing
``` bash
$ composer install
$ composer test
```

## Sponsors

| [![SORGE][img-sorge]][link-sorge]               |
|:-----------------------------------------------:|
| [**SORGE**](link-sorge) - website tracking tool |

## License

This library is released under the Apache 2.0 license. Please see [License File](LICENSE.md) for more information.

[ico-license]: https://img.shields.io/badge/license-Apache2-brightgreen.svg?style=flat-square
[ico-travis]: https://img.shields.io/travis/layershifter/TLDExtract/master.svg?style=flat-square
[ico-php7ready]: http://php7ready.timesplinter.ch/layershifter/TLDExtract/master/badge.svg
[img-sorge]: http://sorge-docs.qdrops.lclients.ru/others/dce4d2dd228406d9376cad60a6c4edb2.png
[link-travis]: https://travis-ci.org/layershifter/gumbo-php
[link-sorge]: http://sorge.pro/eng
