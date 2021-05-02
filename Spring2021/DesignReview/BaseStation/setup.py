import PySimpleGUI as sg
import os
from pathlib import Path


def setup():
    # Variable to control when the GUI is closed
    done = False

    # Initialize our paths to None, to prevent potential error
    current_test_path = None
    baseline_data_path = None
    saved_tests_path = None
    disposable_data_path = None

    # Paths to be defined by the Setup GUI
    sg.theme('Dark Blue 3')

    layout = [[sg.Text('Location to save current tests!', font=("Helvetica", 16), size=(50, 1)), sg.InputText(), sg.FolderBrowse(size=(10, 1),)],
              [sg.Text('Location to save current tests baseline data!', font=("Helvetica", 16), size=(50, 1)), sg.InputText(), sg.FolderBrowse(size=(10, 1),)],
              [sg.Text('Location for any saved test data!', font=("Helvetica", 16), size=(50, 1)), sg.InputText(), sg.FolderBrowse(size=(10, 1), )],
              [sg.Text('Location to save the data for on-site disposable mask!', font=("Helvetica", 16), size=(50, 1)), sg.InputText(), sg.FolderBrowse(size=(10, 1), )],
              [sg.Submit(), sg.Cancel()]]

    window = sg.Window('Settings Menu', layout)

    while not done:
        event, values = window.read()

        current_test_path = values[0]
        baseline_data_path = values[1]
        saved_tests_path = values[2]
        disposable_data_path = values[3]

        # If the user cancels defining save locations, exit, and do not let them attempt to use the main GUI
        if event == 'Cancel' or event == 'None':
            window.close()
            exit()

        # If any of the paths were left undefined, prompt the user to define them all before they may continue
        elif current_test_path == '' or baseline_data_path == '' or saved_tests_path == '' or disposable_data_path == '':
            sg.Popup('Error, not all paths specified! Please fill out all fields')

        else:
            done = True

    window.close()

    # Now create our file
    # First we need to get environment
    environment = os.getenv('APPDATA')

    # Then set the settings path, and open the config file
    settings_path = ((Path(environment)) / 'COVID19-Comparator')
    fd = open((settings_path / 'settings.cfg'), 'w')

    # Now write our paths to the settings config file
    fd.write(current_test_path + '\n')
    fd.write(baseline_data_path + '\n')
    fd.write(saved_tests_path + '\n')
    fd.write(disposable_data_path + '\n')

    # Close the file
    fd.close()

    return
