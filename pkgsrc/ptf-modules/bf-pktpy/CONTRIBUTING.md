# Contributing to bf-pktpy
👍🎉 First off, thanks for taking the time to contribute! 🎉👍

The following is a set of guidelines for contributing to the bf-pktpy in the Intel Barefoot Network organization at the GitHub. 
Feel free to propose changes to this document and our process thru the Pull Request. 

## New issues
We're tracking our changes thru the GtiHub's Issues and the [Jira]().
Proposes changes should be described by the proper issue and related Pull Requests should be linked. 

## Pull Requests
Our code is validated by the few pipelines, but before they will be run, special code format verification starts.
Code style is described by the set of PEP8 and Black. 

Every line of the code should be formatted by the automatic tool Black

To install Black:
```text
pip install black
``` 

To run a code verification:
```text
black --check .
```

If you want to reformat code file
```text
black <your file or .>
```
