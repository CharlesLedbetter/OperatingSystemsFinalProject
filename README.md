# OperatingSystemsFinalProject
This socket based encryption and decryption program was created for
Oregon State University's Operating Systems I course.

To use these programs on windows systems follow this step and
then follow all steps within the Mac or Linux systems section:

1) If it is not already present on your system install PuTTy
   or some other terminal emulator for your Windows system.
   You can find PuTTy at https://www.putty.org.

To use these programs on Mac or Linux systems follow these steps:

1) Clone the repository to your local machine.
2) Navigate to the cloned directory.
3) enter this into the console:

      chmod +x compileall && compileall

4) Create a plain text file (or skip this step if you want to use
   one of the provided plain text files)
5) Enter the following replacing the text between square brackets
(and removing the square brackets):

      otp_enc_d [port number of your choice] &

6) Then enter:

      otp_dec_d [port number of your choice] &

7) The encoding and decoding processes are now active. Next you need to
   create a key at least as long as your plain text file. Do so by
   entering the following, replacing the text between the square brackets
   (and removing the square brackets):

      keygen [number of characters] > [previously unused filename]

8) Now you can encode your plain text file using the following command:

      otp_enc [plain text file] [key file] [opt_enc_d port] > [encoded file]

9) You can decode it using the following command:

      otp_dec [encoded file] [key file] [opt_dec_d port] > [decoded file]
