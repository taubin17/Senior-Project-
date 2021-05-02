import PySimpleGUI as sg
import Testing
from pathlib import Path
import os
import pickle
import setup


def main():

    # Check APPDATA folder for settings. If not present, run the setup to create our settings
    appdata = os.getenv('APPDATA')

    # Ensure the COVID19-Comparator folder exists. If not, make the directory
    COVID19_comparator_path = (Path(appdata) / 'COVID19-Comparator')
    if not os.path.exists(COVID19_comparator_path):
        os.makedirs(COVID19_comparator_path)

    # Now check if our settings file was made. If not, make the settings file
    settings_file_path = COVID19_comparator_path / 'settings.cfg'

    # See if a settings file is present! If not, create one
    if not os.path.exists(settings_file_path):
        print('No settings detected!')
        setup.setup()

    # Open the settings file to get the paths

    fd = open(settings_file_path, 'r')

    # And read in the 4 paths we need for the data. Also, rstrip used to remove trailing newline
    current_test_path = Path(fd.readline().rstrip())
    baseline_data_path = Path(fd.readline().rstrip())
    saved_test_path = Path(fd.readline().rstrip())
    disposable_mask_data = Path(fd.readline().rstrip())

    # Sets theme for GUI
    sg.theme('Dark Blue 3')

    # Sets the layout of GUI
    layout = [[sg.Text("COVID-19 Respirator Results", size=(200, 1), font=("Helvetica", 24), justification='center')],
              [sg.Button("GET THE CURRENT TEST RESULTS", size=(100, 1))],
              [sg.Button("COMPARE TO DISPOSABLE MASK", size=(100, 1))],
              [sg.Button("SET AS DISPOSABLE MASK", size=(100, 1))],
              [sg.Button("SAVE CURRENT TEST", size=(100, 1))],
              [sg.Text("Enter subject name here: ", size=(50, 1)), sg.Input(key='SUBJECT NAME', size=(50, 1))],
              [sg.Button("EXIT", size=(100, 1), button_color='red')],
              [sg.Text('Status: ', size=(6, 1), font=("Helvetica", 14)),
               sg.Text('Stand by', key="Status", font=("Helvetica", 14), size=(60, 1))]
              ]

    # Opens up the GUI
    window = sg.Window("TylerAubin COVID19 Project", layout, size=(800, 275))

    # Waits until test arrives
    test = None

    while True:
        # Get the current event and its value
        event, values = window.read()
        # print(event, values)
        # Exit if the exit button or window is closed
        if event == "EXIT" or event == sg.WIN_CLOSED:
            break

        if event == "GET THE CURRENT TEST RESULTS":
            test = Testing.get_latest_test_score(current_test_path, baseline_data_path)
            test.to_string()
            window.find_element("Status").update("Got latest test!")

            # Boolean used to determine if the current mask was also set to the disposable mask. If so, we dont want to compare tests
            isDisposable = False

        if event == "COMPARE TO DISPOSABLE MASK" and test != None and not isDisposable:
            # Check if there is a disposable mask to compare to
            if not os.path.exists(disposable_mask_data):
                window.find_element("Status").update("Error, no disposable mask set to compare to!")
                return;

            # Open the disposable mask data file and retrieve the score for it
            try:
                disposable_mask_file = open(disposable_mask_data / 'disposable_mask.dat', 'rb')

            except:
                window.find_element("Status").update("Error, no disposable mask set to compare to!")

            disposable_data = pickle.load(disposable_mask_file)

            # Compare the disposable data to our own data
            if (disposable_data.score > test.score):
                window.find_element("Status").update("Disposable mask is more effective, recommend disposable mask!")
            elif (disposable_data.score == test.score):
                window.find_element("Status").update("Tests were equal! (Perhaps disposable was set and compared against itself?)")
            else:
                window.find_element("Status").update("Test subjects mask was more effective than disposable!")

        if event == "SET AS DISPOSABLE MASK" and test != None:

            # Check to ensure the path exists. If not, make it
            if not os.path.exists(Disposable_path):
                os.makedirs(Disposable_path)

            disposable_mask_file = open(Disposable_path / 'disposable_mask.dat', 'wb')
            pickle.dump(test, disposable_mask_file);
            disposable_mask_file.close()
            isDisposable = True
            window.find_element("Status").update("Set the disposable mask!")

        if event == "SAVE CURRENT TEST" and values["SUBJECT NAME"] != '' and test != None:
            print("Saving test!")
            window.find_element("Status").update('Saving File!')
            window.find_element("SUBJECT NAME").update('')
            save_test(test, values, saved_test_path)
            window.find_element("Status").update("File Saved")

        print(event, values)

    window.close()

def save_test(test, values, saved_test_path):
    # If the save file paths do not exist, copy them in
    if not os.path.exists(saved_test_path):
        os.makedirs(saved_test_path)

    print("Opening file to save the data")

    subject_name = values["SUBJECT NAME"]
    saved_test_filename = saved_test_path / "saved_tests.csv"

    saved_test_file = saved_test_filename

    # If the saved_tests file does not exist, make our file, and create our header. Then close the file so
    # it may be opened with the append flag instead
    if not os.path.exists(saved_test_filename):
        print("Creating the saved test file")
        saved_test_file = open(saved_test_file, 'w')
        saved_test_file.write('Subject Name, Temperature Score, Humidity Score, Total Score\n')

    saved_test_file = open(saved_test_filename, 'a')

    # Formats a string to write the test parameters to the test files. Follows the header shown above
    string_to_file = subject_name + ', ' + str(test.temperature_score) + ', ' + str(test.humidity_score) + ', ' + str(test.score) + '\n'

    # Write our data, and close the file
    saved_test_file.write(string_to_file)
    saved_test_file.close()

    return

if __name__ == '__main__':
    main()