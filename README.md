# Project Documentation: Police: Driver's Vehicle Permit Validator

## Introduction
This document provides a comprehensive guide to the "Police: Driver's Vehicle Permit Validator" project. It details the project's setup, how Django operates within it, the role of APIs, and an explanation of the Django folder structure. This project is aimed at law enforcement for validating drivers' vehicle permits through a user interface and barcode scanning.

---

![plot](./documentation/screenshot.png)

## 1. Project Overview
**Purpose:**  
The project is designed to assist law enforcement in validating drivers' vehicle permits. The application allows the addition of drivers via a user interface and the verification of permits through scanning vehicle barcodes.

**Features:**
- User Interface for adding drivers.
- User interface for editing validity of permit.
- Barcode scanning for permit verification.
- Manual barcode verification through the interface should the scanner fail to process scan.

---

## 2. Project Requirements
- **Python 3.12:** Programming language for the backend.
- **Django:** Web framework for the backend server.
- **DjangoRestFramework:** Framework for creating APIs.
- **Arduino/ESP32/C++:** Used for the micro-controller aspect of the project.

---

## 3. Installation and Setup

### Backend Setup:
1. Clone the repository:
    ```shell
    $ git clone https://github.com/LindelwaSfiso/esp32_permit_validator.git
    ```

2. Navigate to the server directory:
    ```shell
    $ cd server
    ```

3. Create a virtual environment and activate it:
    ```shell
    $ python -3.12 -m venv venv
    $ venv\scripts\activate
    ```

4. Install the required packages:
    ```shell
    $ (venv) pip install -r requirements.txt
    ```

5. Apply database migrations:
    ```shell
    $ (venv) manage.py migrate
    ```

6. Run the server:
    ```shell
    $ (venv) manage.py runserver 0.0.0.0:8000
    ```

### Backend CSS Files Setup:
1. Navigate to the server directory:
    ```shell
    $ cd server
    ```

2. Build the CSS files:
    ```shell
    $ npm run build # generates build CSS for the website
    ```

### Creating Administration Account:
1. Navigate to the admin directory:
    ```shell
    $ cd server
    ```

2. Activate the virtual environment:
    ```shell
    $ venv\scripts\activate
    ```

3. Create a superuser account:
    ```shell
    $ (venv) manage.py createsuperuser
    ```

---

## 4. ESP32 Setup
Follow the [ESP32 devtools setup guide](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html) for installing and configuring the ESP32 microcontroller.

---

## 5. Understanding Django

### What is Django?
Django is a high-level Python web framework that encourages rapid development and clean, pragmatic design. It provides various built-in features for database management, user authentication, and much more, making it a powerful tool for building web applications.

### How Django Works in the Project:
1. **Models:** Represent the database schema and structure your data.
2. **Views:** Handle the logic behind the user interface, processing data from models and templates.
3. **Templates:** HTML files that generate the user interface.
4. **URLs:** Routes user requests to the appropriate view.

### APIs in the Project:
DjangoRestFramework is used to create RESTful APIs in this project. These APIs allow communication between the frontend and the backend, enabling features like adding drivers and scanning barcodes.

**Example API Usage:**
- **Adding Drivers:** An API endpoint to add driver information to the database.
- **Verifying Permit Information:** An API endpoint to verify permit information and the owner of the car.
- **Verifying Permits:** An API endpoint to scan and validate vehicle permits.

### Folder Structure of Django:
- **project_root/**
  - **manage.py:** Command-line utility for administrative tasks.
  - **project_name/**
    - **\_\_init\_\_.py:** Marks the directory as a Python package.
    - **settings.py:** Configuration file for the project.
    - **urls.py:** URL declarations for the project.
    - **wsgi.py:** Entry-point for WSGI-compatible web servers.
  - **app_name/**
    - **migrations/**: Database migration files.
    - **admin.py:** Administrative interface configuration.
    - **apps.py:** Application-specific configurations.
    - **models.py:** Database models.
    - **tests.py:** Automated tests.
    - **views.py:** Application logic.

---

## 6. Detailed Workflow of the Project

1. **Driver Addition:**
   - Law enforcement can add drivers through a user-friendly interface.
   - The information is stored in the database using Django models.

2. **Permit Verification:**
   - The application uses a barcode scanner integrated with an ESP32 microcontroller.
   - Scanned data is sent to the Django backend via APIs.
   - The backend verifies the permit against the stored data.

3. **User Authentication:**
   - Django's built-in authentication system manages user login and permissions.
   - Superuser accounts can manage other users and drivers' data.

### Deployment:
- **Backend Deployment:** Deployed on a server, the Django application serves API requests and renders web pages.
- **Frontend Deployment:** Static files, such as CSS and JavaScript, are built using npm and served alongside the backend.


## HTTP: Requests, Responses, and Processing

HTTP (Hypertext Transfer Protocol) is the foundation of any data exchange on the Web. It is a protocol used for transmitting hypertext requests and information between web servers and browsers.

### HTTP Requests
- **Request Line**: Contains a method (like `GET`, `POST`, etc.), a URL, and the HTTP version.
- **Headers**: Additional information sent with the request, like `User-Agent`, `Accept`, etc.
- **Body**: (Optional) Data sent with the request, typically used with methods like `POST`.

### HTTP Responses
- **Status Line**: Contains the HTTP version, status code (like `200 OK`, `404 Not Found`), and a reason phrase.
- **Headers**: Additional information sent with the response, like `Content-Type`, `Set-Cookie`, etc.
- **Body**: (Optional) The content of the response, such as HTML, JSON, etc.

### Processing
1. **Client Sends Request**: The client (usually a browser) sends an HTTP request to the server.
2. **Server Processes Request**: The server processes the request, potentially accessing the database or performing other logic.
3. **Server Sends Response**: The server sends an HTTP response back to the client.

## How Django Serves HTTP Requests and Responses

Django is a high-level Python web framework that simplifies the process of building web applications.

### Request Handling
1. **URL Routing**: Django uses a URL dispatcher to direct incoming HTTP requests to the appropriate view based on the URL pattern.
2. **View Processing**: The view function handles the request, performs any necessary logic (e.g., querying the database), and prepares an HTTP response.
3. **Template Rendering**: If the view uses a template, Django's template engine renders the HTML content.

### Response Handling
1. **Creating a Response**: The view function creates an `HttpResponse` object containing the content and headers.
2. **Returning the Response**: The `HttpResponse` object is returned to the client, completing the request-response cycle.

## How Django Talks to the Database

Django provides an ORM (Object-Relational Mapping) system that allows developers to interact with the database using Python code.

### Models
- **Defining Models**: Models are Python classes that define the schema of the database tables.
- **Querying Models**: Developers can perform CRUD (Create, Read, Update, Delete) operations using Django's ORM methods.

### Database Interaction
1. **Configuring Database**: The database configuration is defined in Django's settings (`settings.py`), including the database engine, name, user, password, host, and port.
2. **Migrations**: Django uses migrations to manage changes to the database schema. Developers create migration files and apply them to update the database.
3. **Querying the Database**: When a view function needs to access the database, it uses Django's ORM to query the models and retrieve or modify data.

Here's a simplified example of how a view might query the database and return an HTTP response:

```python
from django.shortcuts import render
from .models import MyModel

def my_view(request):
    # Query the database
    data = MyModel.objects.all()
    
    # Render a template with the data
    return render(request, 'my_template.html', {'data': data})
```

## How TailwindCSS Works

The project uses TAilwindCSS for styling.
TailwindCSS is a utility-first CSS framework that provides low-level utility classes to build responsive user interfaces. Instead of writing custom CSS, developers apply these utility classes directly to HTML elements.

### Core Concepts
- **Utility Classes**: TailwindCSS offers a wide range of utility classes for styling elements, such as `text-center`, `bg-blue-500`, `py-4`, etc.
- **Responsive Design**: Classes can be prefixed with responsive breakpoints (e.g., `md:text-lg`) to apply styles at different screen sizes.
- **Customization**: TailwindCSS can be customized through the `tailwind.config.js` file to define colors, fonts, spacing, and more.
- **Arbitrary Values**: TailwindCSS allows for arbitrary values using square bracket notation (e.g., `top-[117px]`).

### Example
```html
<div class="flex items-center space-x-2 text-base">
  <h4 class="font-semibold text-slate-900">Contributors</h4>
  <span class="rounded-full bg-slate-100 px-2 py-1 text-xs font-semibold text-slate-700">204</span>
</div>
```

---

## 7. References and Resources
1. [Django Documentation](https://docs.djangoproject.com/)
2. [DjangoRestFramework Documentation](https://www.django-rest-framework.org/)
3. [ESP32 Setup Guide](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)
4. [QR Code Scanner with ESP32 CAM Module & OpenCV](https://iotprojectsideas.com/qr-code-scanner-with-esp32-cam-module-opencv/)


```shell
$ cd ewscsla
$ venv\scripts\activate
$ (venv) manage.py createsuperuser
```

### ESP32 Set-up

Follow this guide to install and set-up the esp32 devtools:
https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html

### Libraries
1. Keypad
2. Arduino version 1.8 
3. ESP32 version 2.0.17


# References

1. https://iotprojectsideas.com/qr-code-scanner-with-esp32-cam-module-opencv/
2. https://github.com/GDanovski/ESP32CameraI2S
3. https://bitluni.net/esp32-i2s-camera-ov7670
