.text
.global __divide_error
.type __divide_error, @function
.p2align 5
__divide_error:
    mov $0x0, %bl
    idiv %bl
