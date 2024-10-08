# Game Application

## Overview

This application is a simple game that initializes necessary components, sets up game logic, and allows the player to progress through levels until they win or lose. The game provides feedback through visual indicators and handles user input for gameplay.

## Features

- **Level Progression**: Players can advance through multiple levels, each with increasing difficulty.
- **RGB Color Feedback**: The application uses RGB colors to indicate game status:
  - **Blue**: Game not in progress
  - **Green**: Game started with 3 lives remaining
  - **Olive Green**: 2 lives remaining
  - **Reddish Orange**: 1 life remaining
  - **Red**: Game over (0 lives remaining)
- **User Input**: Players must enter sequences or words correctly to progress through the use of a Raspberry Pico Pi. These button presses are registered with ARM Assembly.
- **Winning and Losing Conditions**: The game ends when the player wins at level 5 or loses all lives.

## Authors

- Matthew Power
- Dylan Gallagher
- Fareedah Hafis-Omotayo
- Stephen Komolafe
- Palak Aggarwal
