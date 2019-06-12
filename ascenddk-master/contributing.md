# Contributing<a name="EN-US_TOPIC_0140918376"></a>

Huawei Ascend is the supporting software and tools for Atlas 200 Developer Kit open sourced by Huawei, we welcome and encourage the community to do contribution on our project. Contribution includes but not limited to bug reports, feature requests, code patches and so on.

## Issue<a name="section18258134285"></a>

## Bug Reports<a name="section8416262287"></a>

For bug reports, we suggest the community to use following template to report all kinds of bugs. When creating a new issue, you could select the bug reports option and the following template will pop up, then you should fill items according to the instructions.

-   **Describe the bug**

    _A clear and concise description of what the bug is._

-   **To Reproduce**

    _Steps to reproduce the behavior_:

    1.  step1
    2.  step2
    3.  step3
    4.  step4

-   **Expected behavior**

    _A clear and concise description of what you expected to happen._XXX

-   **Current behavior**

    _A clear and concise description of what happens currently._

-   **Your environment:**
    -   AscendDK Version:
    -   Mind Studio Version:
    -   Developer Kit Version:

-   **Other information**  \(_e.g. detailed explanation, stacktraces, related issues, suggestions how to fix, etc_\)

## Feature Requests<a name="section6302131652711"></a>

For feature requests, we suggest the community to use following template to submit suggestions for this project. Just like creating the bug reports, the only difference is that you should select the feature requests option instead.

-   **Is your feature request related  to a problem? Please describe.**

    _A clear and concise description of what the problem is. Ex. I'm always frustrated when ..._

-   **Describe the solution you'd like**

    _A clear and concise description of what you want to happen._

-   **Describe alternatives you've considered**

    _A clear and concise description of any alternative solutions or features you've considered._

-   **Other information**

    _Add any other context or screenshots about the feature request here._


## Code Patches<a name="section149206221252"></a>

## Contribution Workflow<a name="section53381699363"></a>

For code patches, the community may need take several actions to finish this kind of contribution. Contribution workflow will be described step by step as following instructions:

1.  Pick Issue

    Pick one open issue from the issues list and add some comments on it to inform the community that you have already made a proposal to solve this issue.

2.  Fork Asecnddk

    Create a fork of Asecnddk by using your own github account, you could login github, click the  **fork button**  and choose repository position.

3.  Clone Asecnddk

    You have already createn a new repository and now you could just clone it to your local machine by using following command:

    git clone https://github.com/<your github id\>/Ascenddk.git

4.  Add Upstream

    To fetch latest update from offical Ascenddk repository, you should add the offical repo as remote repo by using following command, here we name it as upstream:

    git remote add upstream  [https://github.com/ascenddk/ascenddk.git](https://github.com/ascenddk/ascenddk.git)

5.  Create Working Branch

    To make sure the paches code clean, we suggest you to create a brand new branch for certain issue, and we also suggest that name it by issue number\(we use issue\#\#\# here as example\), you could using following command:

    git checkout develop

    git checkout -b issue\#\#\# origin/develop

6.  Make Change

    Make change locally and test your patch to make sure that you have already solve this issue you picked at step 1.

7.  Add Changed Files

    Add the changed files to modified list which will be committed soon:

    **git add \[changed files\]**

8.  Commit Changes Locally

    We suggest using the commit message template to make sure git log maintained normally. Git commit message template will be described later. The following command which is used without "-m" option will invoke commit message templates automatically:

    **git commit**

9.  Push Changes

    Push the code changes on working branch to your own fork by using following command:

    **git push origin issue\#\#\#**

10. Create Pull Request

    Now you could go to your own fork repo page at github, and create a pull request. Here is the  <u>[pull request creation instructions](https://help.github.com/articles/creating-a-pull-request-from-a-fork/)</u>.


## Commit Message<a name="section121824115433"></a>

## Commit Message Template<a name="section1685412024314"></a>

-   The Layout

    ```
    <type>: <subject>
    <body>
    <footer>
    ```

-   The Title

    The title consists of the <type\> and the <subject\>. The first one is used to describe what kind of work will be commited for this operation. And the second one is used to describe what this commit does, no more than 50 charactors.

    For <type\>section, following values are allowed:

    -   **feat**  \(new feature\)
    -   **fix**  \(bug fix\)
    -   **docs**  \(changes to documentation\)
    -   **style**  \(formatting, missing semi    colons, etc; no code change\)
    -   **refactor**  \(refactoring production    code\)
    -   **test**  \(adding missing tests,    refactoring tests; no production code change\)
    -   **chore**  \(updating grunt tasks etc;    no production code change\)

-   The Body

    The commit body must contain the messages which are used to state what the issue is that you worked on, and  **what**  your code changes and fix entailed. The fix, or the  **why**  of your code is really interesting, as well as important, and helps others understand the solution you chose.

-   The Footer

    Here you can reference issues and pull-requests that relate to your commit, like so:

    ```
    closes #125
    ```

    Or the footer could also describe the imcompatible changes by using BREAKING CHANGES at the very beginning of the first footer line and followed by several line of reasons.


## Setting Gitconfig<a name="section4966441154817"></a>

Assume that the commit message template is store as .gitmessage in root folder of the repository. You could use following commands to set the .gitmessage as commit template.

-   Set Template

    **git config --global commit.template <path to .gitmessage\>**


-   Set Editor

    Chosse your favourite editor to fill the commit template.

    **git config --global core.editor vim**

-   编码规范

    google


