# Проект Harakiri

Команда: Приматы
Задача: предсказание аномалий по битрейту h264 без декодирования
Отель: Триваго

## Как сбилдить

### Без докера

Системные зависимости: _ffmpeg_, _sdl2_.

```bash
mkdir build
cd build
cmake ..
cd ..
cmake --build build -j2
```

Запускаем бинарник:
```bash
./build/harakiri
```

### С докером

TODO
