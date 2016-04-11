import qbs

Project {
    name: "statemachine"
    references: [
        "eventtransitions",
        "factorial",
        "pingpong",
        "rogue",
        "trafficlight",
        "twowaybutton",
    ]
}
