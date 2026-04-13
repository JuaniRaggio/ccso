// ============================================================================
// ChompChamps -- Operating Systems Report
// ============================================================================

#set document(
  title: "ChompChamps -- Inter Process Communication",
  author: ("Santiago Cibeira", "Victoria Helena Park", "Juan Ignacio Garcia Vautrin Raggio"),
)

#set text(font: "New Computer Modern", size: 11pt, lang: "en")
#set par(justify: true)
#set heading(numbering: "1.1")

// ---------------------------------------------------------------------------
// Cover page
// ---------------------------------------------------------------------------

#page(numbering: none, margin: (top: 3cm, bottom: 3cm, left: 2.5cm, right: 2.5cm))[
  #align(center)[
    #v(2cm)

    #text(size: 14pt, weight: "bold")[Instituto Tecnologico de Buenos Aires]

    #v(1cm)

    #line(length: 60%, stroke: 0.5pt)

    #v(1.5cm)

    #text(size: 24pt, weight: "bold")[ChompChamps]

    #v(0.4cm)

    #text(size: 16pt)[Inter Process Communication]

    #v(0.6cm)

    #text(size: 13pt, style: "italic")[Operating Systems -- Assignment 1]

    #v(1.5cm)

    #line(length: 60%, stroke: 0.5pt)

    #v(2cm)

    #text(size: 12pt)[
      *Santiago Cibeira* \
      *Victoria Helena Park* \
      *Juan Ignacio Garcia Vautrin Raggio*
    ]

    #v(1.5cm)

    #text(size: 11pt)[First Semester -- 2025]
  ]
]

// ---------------------------------------------------------------------------
// Table of contents
// ---------------------------------------------------------------------------

#page(numbering: "i")[
  #outline(title: "Table of Contents", indent: 1.5em, depth: 3)
]

// ---------------------------------------------------------------------------
// Body -- page numbering restarts at 1
// ---------------------------------------------------------------------------

#set page(numbering: "1")
#counter(page).update(1)

// ============================= 1. Introduction =============================

= Introduction

// Brief overview of the project: what ChompChamps is, the goal of the
// assignment, and the IPC concepts it exercises.

// TODO: fill in

// ============================ 2. Architecture ==============================

= Architecture

== Overview

// High-level description of the three-process model (master, player, view)
// and how they interact.

// TODO: fill in

== Process Diagram

// Include or describe a diagram showing the master, players, view, shared
// memory regions, pipes, and semaphore flow.

// TODO: fill in

// ======================== 3. Master Process ================================

= Master Process

== Responsibilities

// Enumerate the master's duties: create shared memory, spawn processes,
// manage the game loop, validate moves, enforce timeout, etc.

// TODO: fill in

== Command-Line Parameters

// Document all accepted flags (-w, -h, -d, -t, -s, -v, -p) with defaults.

// TODO: fill in

== Game Loop

// Describe the main loop: select() multiplexing, round-robin scheduling,
// move validation, view synchronization, delay, and timeout.

// TODO: fill in

== Process Spawning

// Explain how the master forks players and the view, sets up pipes and
// passes arguments.

// TODO: fill in

== Endgame and Cleanup

// How the master detects game-over, waits for children, prints results,
// and cleans up shared memory and semaphores.

// TODO: fill in

// ======================== 4. Player Process ================================

= Player Process

== Responsibilities

// General overview: connect to shared memory, read board, compute move,
// send via pipe, wait for next turn.

// TODO: fill in

== Synchronization Protocol

// Detailed description of the player's interaction with the semaphore
// array (token consumption, reader lock, pipe write, turn wait).

// TODO: fill in

== AI Strategies

// Document each implemented strategy.

=== Naive (Random)

// TODO: fill in

=== Greedy

// TODO: fill in

=== Greedy Lookahead

// TODO: fill in

=== Flood Fill

// TODO: fill in

=== Greedy Flood (Hybrid)

// TODO: fill in

=== Min Reward

// TODO: fill in

// ========================= 5. View Process =================================

= View Process

== Responsibilities

// Overview: connect to shared memory, wait for render signal, display
// board and player stats, notify master when done.

// TODO: fill in

== Rendering

// Describe the ncurses-based UI: board layout, color coding, player
// indicators, score display, resize handling.

// TODO: fill in

// =================== 6. Inter-Process Communication ========================

= Inter-Process Communication

== Shared Memory

// Describe the two shared memory regions: /game_state and /game_sync.
// Explain creation, mapping, layout, and cleanup.

// TODO: fill in

=== Game State (`/game_state`)

// Detail the game_state_t and player_t structures: fields, board encoding
// (1..9 for rewards, -id for captured cells), coordinate system.

// TODO: fill in

=== Synchronization State (`/game_sync`)

// Detail the game_sync_t structure: all semaphores, readers_count, and
// per-player turn gates.

// TODO: fill in

== Semaphores

// Explain each semaphore's role and initial value.

// TODO: fill in

=== Readers-Writers Lock

// Describe the readers-writers pattern with writer-starvation prevention.
// Cover the turnstile semaphore, reader count mutex, and game state mutex.

// TODO: fill in

=== View Synchronization

// Describe the bidirectional signaling between master and view
// (view_may_render / view_rendered).

// TODO: fill in

=== Player Turn Control

// Describe the per-player semaphore array and how turns are granted and
// consumed.

// TODO: fill in

== Pipes

// Describe the anonymous pipes used for movement requests: one per player,
// 1-byte direction encoding (0..7), master reads via select().

// TODO: fill in

// ======================= 7. Data Structures ================================

= Data Structures

== `player_t`

// Document all fields: name, score, valid/invalid move counts, coordinates,
// pid, blocked flag.

// TODO: fill in

== `game_state_t`

// Document fields: width, height, player count, player array, running flag,
// flexible array member for the board.

// TODO: fill in

== `game_sync_t`

// Document all semaphore fields and the readers_count variable.

// TODO: fill in

== Board Encoding

// Explain the cell value encoding: 1..9 = reward, 0..-8 = captured by
// player -id. Row-major layout.

// TODO: fill in

== Movement Protocol

// Describe direction encoding (0 = UP, clockwise to 7 = UP_LEFT),
// delta table, and invalid move handling.

// TODO: fill in

// ====================== 8. Build System ====================================

= Build System

== Docker Environment

// Explain the mandatory Docker image (agodio/itba-so-multiarch:3.1),
// why macOS native builds are not supported, and how to use docker-build.sh.

// TODO: fill in

== Makefile Targets

// Document key targets: all, master, players, view, test, memcheck, run.

// TODO: fill in

== Compilation Flags

// Describe -Wall, -g, -lpthread, -lrt, and per-strategy -D flags.

// TODO: fill in

== Player Strategy Compilation

// Explain how each strategy is compiled as a separate binary via -D flags
// and the Makefile name mapping.

// TODO: fill in

// ========================= 9. Testing ======================================

= Testing

== Framework

// Describe the CuTest framework and test infrastructure.

// TODO: fill in

== Test Coverage

// List all test suites and what they cover: parser, game_admin, game_sync,
// player_movement, player_protocol, error_management.

// TODO: fill in

== Memory Analysis

// Describe Valgrind integration (make memcheck), leak-check results, and
// any suppressions needed.

// TODO: fill in

// ==================== 10. Design Decisions =================================

= Design Decisions

// Justify key architectural and implementation choices.

== Shared Memory over Other IPC

// Why shared memory was chosen for game state distribution.

// TODO: fill in

== Readers-Writers with Writer Priority

// Why the writer-starvation-free variant was selected.

// TODO: fill in

== `select()` for Pipe Multiplexing

// Why select() was used instead of threads or poll().

// TODO: fill in

== Per-Strategy Compilation

// Why strategies are selected at compile time rather than runtime.

// TODO: fill in

== Semaphore Initialization Values

// Explain the initial values of each semaphore and the rationale
// (e.g., player_may_send_movement initialized to UNLOCKED).

// TODO: fill in

// ======================== 11. Limitations ==================================

= Limitations

// Document known limitations: maximum player count, board size constraints,
// platform dependencies, etc.

// TODO: fill in

// =============== 12. Problems and Solutions =================================

= Problems and Solutions

// Describe challenges encountered during development and how they were
// resolved (e.g., macOS incompatibilities, deadlock debugging, race
// conditions, semaphore initialization alignment with the provided binary).

// TODO: fill in

// ======================= 13. Conclusion ====================================

= Conclusion

// Summarize the project, lessons learned about IPC, and overall results.

// TODO: fill in

// ======================== 14. References ===================================

= References

// List references: man pages (shm_overview(7), sem_overview(7), pipe(7),
// select(2)), course materials, and any external resources used.

// TODO: fill in

// ====================== 15. AI Usage =======================================

= AI Usage Disclosure

// Document any use of AI tools during development, as required by the
// assignment. Include which tools, what for, and to what extent.

// TODO: fill in
