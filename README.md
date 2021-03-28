# napi_sha2_async
## мотивация

Я не нашел ниодной многопоточной библиотеки для хэширования для nodejs, и потому написал свою

## install

    npm i hu2prod/napi_sha2_async

## usage

 * see ./test.coffee
 * Есть ряд случаев, когда лучше отказаться от использования этой библиотеки
   * Если хэши выполняются последовательно (например, нужно посчитать один несбалансированный merkle root)
   * Если параллельных запросов недостаточно много (хотя бы в 2 раза больше чем UV_THREADPOOL_SIZE)
   * Если нужно хэшировать мелкие объекты (<2 Кб)
 * `UV_THREADPOOL_SIZE=$(nproc) ./test.coffee` будет несколько быстрее чем по умолчанию
   * Но больше thread'ов не всегда лучше
 * В модуле есть еще sha2_sync, но зачем, если есть crypto

## benchmark

    UV_THREADPOOL_SIZE=32
    
    in_buf_size 80
    seq base 10079.92
    par mod  5044.51
    no_alloc 6818.18
    
    in_buf_size 1024
    seq base 7195.61
    par mod  5039.53
    no_alloc 6448.41
    
    in_buf_size 2048
    seq base 5254.75
    par mod  5615.76
    no_alloc 6274.90
    
    in_buf_size 10240
    seq base 1791.04
    par mod  4489.02
    no_alloc 5677.29
    
    in_buf_size 102400
    seq base 206.08
    par mod  1622.14
    no_alloc 1582.59
