FROM python:3.9-slim-buster

# set a directory for the app
WORKDIR /app

# copy all the files to the container
COPY . .

# install dependencies
RUN pip install -r requirements.txt

# run the command
CMD ["python", "./realtime_firebase_data.py"]