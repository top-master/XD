import qbs

Project {
    name: "threads"
    references: [
        "mandelbrot",
        "semaphores",
        "waitconditions",
    ]
}
