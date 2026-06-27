# Airplane_Reservation_Simulator

The **Domestic Airplane Reservation Simulator** is a command-line based system developed entirely in **Modern C++ (C++17)**. It simulates the core operations of a domestic airline reservation platform, incorporating robust **Object-Oriented Design Principles**, **Data Structure Optimization**, and **real-world airline workflow** simulation. This project offers both passenger- and admin-facing functionalities with high levels of modularity, validation, and persistence.

---
## Table of Contents

- [Project Overview](#project-overview)
- [Key Features](#key-features)
  - [Passenger Module](#passenger-module)
  - [Admin Panel](#admin-panel)
- [Technical Architecture](#technical-architecture)
- [Project Structure](#project-structure)
- [Data Persistence](#data-persistence)
- [Input Validation & Error Handling](#input-validation--error-handling)
- [Distinctive Aspects](#distinctive-aspects)
- [Contribution Guidelines](#contribution-guidelines)
- [Conclusion](#conclusion)
- [Author Information](#author-information)

---
## Project Overview

This system was developed with the following key objectives:

- Comprehensive airline reservation simulation
- Modular and scalable system design
- Efficient runtime performance using STL containers
- Seamless data persistence across sessions
- Robust input validation and exception-safe coding practices

---

## Key Features

### Passenger Module

- **Secure Aadhaar-based Login** with 12-digit validation
- Search available flights by **source**, **destination**, and **date**
- **Live seat visualization** using a 40x6 seating layout
- Interactive ticket booking with input confirmation and correction
- Real-time **fare calculation** and seat availability checks
- **Ticket cancellation** with automated refund policy:
  - Full refund if canceled more than 7 days before the journey
  - 50% refund if canceled within 7 days
  - No refund on the day of travel
- **Boarding pass generation** with complete travel and passenger details
- Detailed **booking history with timestamps**

---

### Admin Panel

- **Username and password-based login**
- View and manage all available flights
- Access **real-time occupancy** and **revenue reports**
- Display all booking records with seat-level details
- Monitor **waitlisted passengers** and **priority boarding queues**
- All changes reflect instantly across modules

---

## Technical Architecture

The simulator emphasizes performance and clean architecture by leveraging the following:

|      Component          |      Implementation Strategy        |
|-------------------------|-------------------------------------|
| Data Access             | `std::map`, `std::unordered_map`    |
| Dynamic Collections     | `std::vector`                       |
| Queuing Systems         | `std::queue`, `std::priority_queue` |
| Unique Data Constraints | `std::set`                          |
| Time and Date Handling  | `std::chrono`, `std::tm`            |

Each data structure is carefully selected based on its average-case performance characteristics to ensure minimal time complexity across key operations.

---

## Project Structure
```
/Airplane-Reservation-Simulator
├── /include
│ ├── Admin.h
│ ├── BoardingPass.h
│ ├── BookingSystem.h
| ├── Flight.h
│ ├── LoginManager.h
│ ├── Pricing.h
│ ├── Passenger.h
│ ├── RouteManager.h
| ├── VoiceAssistant.h
├── /src
│ ├── Admin.cpp
│ ├── BoardingPass.cpp
│ ├── BookingSystem.cpp
| ├── Flight.cpp
│ ├── LoginManager.cpp
│ ├── Pricing.cpp
│ ├── Passenger.cpp
│ ├── RouteManager.cpp
| ├── VoiceAssistant.cpp
├── Project_Report.pdf
├── README.md
├── admin_credentials.txt
├── bookings.txt
├── flights.txt
└── main.cpp
```

---

## Data Persistence

- Booking data is maintained in `bookings.txt`
- Flight metadata is stored in `flights.txt`
- All records persist between executions
- All inputs undergo thorough validation
- Incorrect entries are gracefully handled and re-prompted at the field level

---

## Input Validation & Error Handling

This system applies rigorous validation at every point of user interaction:

- Aadhaar must be exactly 12 digits
- City names are matched case-insensitively
- Invalid data types or out-of-range values trigger contextual prompts without system termination
- Users can revise inputs interactively before confirmation
- Gender selection and age are type-checked and restricted to valid entries

---

## Distinctive Aspects

Unlike many existing reservation simulators, this project:

- Is **entirely developed from scratch in C++17** without reliance on external libraries
- Implements **pure Object-Oriented Design** with minimal coupling and high cohesion
- Prioritizes **algorithmic efficiency** and **data integrity**
- Provides a **realistic, user-friendly experience** on both passenger and administrative fronts

Its clean modular structure also makes it an excellent reference for learning system design, data handling, and CLI application development in C++.

---

## Contribution Guidelines

This project was developed independently as a demonstration of advanced programming concepts. However, contributions, suggestions, and pull requests are welcome.

- Fork the repository
- Create feature branches for enhancements or bug fixes
- Ensure your changes align with the modular class design and file structure
- Submit a pull request with a detailed description of modifications

---

## Conclusion

The **Domestic Airline Reservation Simulator** represents a culmination of applied data structures, system design principles, and object-oriented programming. Its architecture allows seamless extension and adaptation to broader scopes such as international travel systems, payment gateways, and customer tiering.

The system emphasizes:

- **Practical implementation of core CS principles**
- **Scalable structure** for real-world adaptation
- **Robust validation mechanisms**
- **Comprehensive simulation of airline reservation systems**

---

## Author Information

**Pranav Ram**
GitHub: [@Pranav-Ram-R](https://github.com/Pranav-Ram-R)

---

> *"A well-architected system is more than code; it is the embodiment of clarity, correctness, and creativity."*
