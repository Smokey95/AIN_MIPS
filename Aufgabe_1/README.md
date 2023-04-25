# Task 1

In this task, you should first familiarize yourself with the development environment. Then the functionality for debouncing a button and flashing an LED with different patterns will be realized.


## LED pattern flashing `TA0.c/.h`

```plantuml
@startuml
binary "Enable" as EN

@0
EN is low

@5
EN is high

@10
EN is low
@enduml
```

## Button debouncing `TA1.c/.h`