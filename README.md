# feedbacktool

This is a simple feedback tool, developped to get anonymous feedback to a server
(e.g. for teaching) through this specific command line program (client),
communicating over a simple TCP connection. should run without problems under at least most of the x64 linux os

For using you have to run one server first. for starting you need to specify the servers public port (pay attention to firewalls!)
where it should run and a file with the questions for the feedback, which are commited to the clients. One line is one question,
exept the first and the last one, which are only messages to the client. A template is in the 'server' folder.

Then ask the people you are requesting feedback from to run the client,
which they should start with your servers DNS name or IP and the public port.

The feedback will be collected in a file in the directory where the server runs, it's called 'year_mm_dd_feedback.csv'

It's free software, feel free to use, share and modify it, a copy of the license, the GPLv3 is attached.

Have fun :)
