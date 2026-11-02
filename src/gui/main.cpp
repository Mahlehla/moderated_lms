#include "data/Database.hpp"
#include "data/UserRepository.hpp"
#include "data/DepartmentRepository.hpp"
#include "data/CourseRepository.hpp"
#include "data/EnrolmentRepository.hpp"
#include "data/AdministrationRepository.hpp"
#include "data/AssignmentRepository.hpp"
#include "data/SubmissionRepository.hpp"
#include "services/AssessmentWorkflowService.hpp"
#include "services/AuthenticationService.hpp"
#include "domain/User.hpp"
#include "utils/DatabasePath.hpp"

#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDateTimeEdit>
#include <QDoubleSpinBox>
#include <QTextEdit>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>

#include <memory>
class DashboardWindow : public QDialog {
public:
    DashboardWindow(
        const User& user,
        UserRepository& users,
        DepartmentRepository& departments,
        CourseRepository& courses,
        EnrolmentRepository& enrolments,
        AdministrationRepository& administration,
        AssignmentRepository& assignments,
        SubmissionRepository& submissions,
        AssessmentWorkflowService& assessment
    )
        : user_(user),
          users_(users),
          departments_(departments),
          courses_(courses),
          enrolments_(enrolments),
          administration_(administration),
          assignments_(assignments),
          submissions_(submissions),
          assessment_(assessment)
    {
        setWindowTitle("Moderated LMS - Dashboard");
        resize(1050, 700);

        auto* root = new QVBoxLayout(this);

        auto* welcome = new QLabel(
            QString("Welcome, %1 | Role: %2")
                .arg(QString::fromStdString(user_.fullName()))
                .arg(QString::fromStdString(roleToString(user_.role())))
        );

        welcome->setStyleSheet(
            "font-size: 18px; font-weight: bold; padding: 10px;"
        );

        root->addWidget(welcome);

        if (user_.role() == Role::Administrator) {
            buildAdministratorDashboard(root);
        } else if (user_.role() == Role::Lecturer) {
            buildLecturerDashboard(root);
        } else if (user_.role() == Role::Student) {
            buildStudentDashboard(root);
        } else {
            auto* message = new QLabel(
                QString::fromStdString(user_.permissionsSummary())
                + "\n\nThis role's dashboard will be implemented "
                  "in the next development stage."
            );
            message->setWordWrap(true);
            root->addWidget(message);
            root->addStretch();
        }

        auto* logout = new QPushButton("Log out");
        root->addWidget(logout);

        connect(logout, &QPushButton::clicked,
                this, &QDialog::accept);
    }

private:
    const User& user_;
    UserRepository& users_;
    DepartmentRepository& departments_;
    CourseRepository& courses_;
    EnrolmentRepository& enrolments_;
    AdministrationRepository& administration_;
    AssignmentRepository& assignments_;
    SubmissionRepository& submissions_;
    AssessmentWorkflowService& assessment_;

    QTabWidget* tabs_{};
    QLabel* userCount_{};
    QLabel* departmentCount_{};
    QLabel* courseCount_{};

    QTableWidget* userTable_{};
    QTableWidget* departmentTable_{};
    QTableWidget* courseTable_{};

    QLineEdit* usernameInput_{};
    QLineEdit* fullNameInput_{};
    QLineEdit* passwordInput_{};
    QComboBox* roleInput_{};

    QLineEdit* departmentNameInput_{};

    QLineEdit* courseCodeInput_{};
    QLineEdit* courseNameInput_{};
    QComboBox* courseDepartmentInput_{};
    QComboBox* lecturerInput_{};
    QComboBox* lecturerCourseInput_{};
    QComboBox* hodInput_{};
    QComboBox* hodDepartmentInput_{};
    QComboBox* studentInput_{};
    QComboBox* studentCourseInput_{};

    static QTableWidget* createTable(
        const QStringList& headers,
        QWidget* parent
    )
    {
        auto* table = new QTableWidget(parent);
        table->setColumnCount(headers.size());
        table->setHorizontalHeaderLabels(headers);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->horizontalHeader()->setStretchLastSection(true);
        table->verticalHeader()->setVisible(false);
        return table;
    }


    void buildStudentDashboard(QVBoxLayout* root)
    {
        auto* studentTabs = new QTabWidget(this);

        // My Courses tab
        auto* coursesPage = new QWidget;
        auto* coursesLayout = new QVBoxLayout(coursesPage);

        auto* coursesHeading = new QLabel("My Enrolled Courses");
        coursesHeading->setStyleSheet(
            "font-size: 16px; font-weight: bold; padding: 6px;"
        );

        auto* studentCoursesTable = createTable(
            {"Course ID", "Course Code", "Course Name"},
            coursesPage
        );

        // Available Assignments tab
        auto* assignmentsPage = new QWidget;
        auto* assignmentsLayout = new QVBoxLayout(assignmentsPage);

        auto* assignmentsHeading = new QLabel(
            "Released Assignments"
        );
        assignmentsHeading->setStyleSheet(
            "font-size: 16px; font-weight: bold; padding: 6px;"
        );

        auto* studentAssignmentsTable = createTable(
            {
                "Assignment ID",
                "Course",
                "Title",
                "Description",
                "Deadline",
                "Maximum Mark"
            },
            assignmentsPage
        );

        const std::vector<int> enrolledCourseIds =
            enrolments_.findCoursesForStudent(user_.id());

        const std::vector<Course> allCourses = courses_.findAll();

        int courseRow = 0;
        int assignmentRow = 0;

        for (const int courseId : enrolledCourseIds) {
            for (const Course& course : allCourses) {
                if (course.id() != courseId) {
                    continue;
                }

                studentCoursesTable->insertRow(courseRow);

                studentCoursesTable->setItem(
                    courseRow, 0,
                    new QTableWidgetItem(QString::number(course.id()))
                );
                studentCoursesTable->setItem(
                    courseRow, 1,
                    new QTableWidgetItem(
                        QString::fromStdString(course.code())
                    )
                );
                studentCoursesTable->setItem(
                    courseRow, 2,
                    new QTableWidgetItem(
                        QString::fromStdString(course.name())
                    )
                );

                ++courseRow;

                const std::vector<Assignment> courseAssignments =
                    assignments_.findByCourse(courseId);

                for (const Assignment& assignment : courseAssignments) {
                    if (!assignment.released()) {
                        continue;
                    }

                    studentAssignmentsTable->insertRow(assignmentRow);

                    studentAssignmentsTable->setItem(
                        assignmentRow, 0,
                        new QTableWidgetItem(
                            QString::number(assignment.id())
                        )
                    );
                    studentAssignmentsTable->setItem(
                        assignmentRow, 1,
                        new QTableWidgetItem(
                            QString::fromStdString(course.code())
                        )
                    );
                    studentAssignmentsTable->setItem(
                        assignmentRow, 2,
                        new QTableWidgetItem(
                            QString::fromStdString(assignment.title())
                        )
                    );
                    studentAssignmentsTable->setItem(
                        assignmentRow, 3,
                        new QTableWidgetItem(
                            QString::fromStdString(
                                assignment.description()
                            )
                        )
                    );
                    studentAssignmentsTable->setItem(
                        assignmentRow, 4,
                        new QTableWidgetItem(
                            QString::fromStdString(assignment.deadline())
                        )
                    );
                    studentAssignmentsTable->setItem(
                        assignmentRow, 5,
                        new QTableWidgetItem(
                            QString::number(assignment.maxMark())
                        )
                    );

                    ++assignmentRow;
                }

                break;
            }
        }

        if (courseRow == 0) {
            coursesLayout->addWidget(
                new QLabel("You are not enrolled in any courses yet.")
            );
        }

        if (assignmentRow == 0) {
            assignmentsLayout->addWidget(
                new QLabel("There are no released assignments available.")
            );
        }

        coursesLayout->addWidget(coursesHeading);
        coursesLayout->addWidget(studentCoursesTable);

        assignmentsLayout->addWidget(assignmentsHeading);
        assignmentsLayout->addWidget(studentAssignmentsTable);

        studentTabs->addTab(coursesPage, "My Courses");
        studentTabs->addTab(assignmentsPage, "Available Assignments");

        root->addWidget(studentTabs);
    }

    void buildLecturerDashboard(QVBoxLayout* root)
    {
        auto* lecturerTabs = new QTabWidget(this);

        // My Courses tab
        auto* coursesPage = new QWidget;
        auto* coursesLayout = new QVBoxLayout(coursesPage);

        auto* coursesHeading = new QLabel("Courses assigned to me");
        coursesLayout->addWidget(coursesHeading);

        auto* lecturerCoursesTable = createTable(
            {"Course ID", "Code", "Course Name", "Department ID"},
            coursesPage
        );

        const auto lecturerCourses = courses_.findByLecturer(user_.id());

        for (const auto& course : lecturerCourses) {
            const int row = lecturerCoursesTable->rowCount();
            lecturerCoursesTable->insertRow(row);

            lecturerCoursesTable->setItem(
                row, 0, new QTableWidgetItem(QString::number(course.id()))
            );
            lecturerCoursesTable->setItem(
                row, 1, new QTableWidgetItem(QString::fromStdString(course.code()))
            );
            lecturerCoursesTable->setItem(
                row, 2, new QTableWidgetItem(QString::fromStdString(course.name()))
            );
            lecturerCoursesTable->setItem(
                row, 3, new QTableWidgetItem(
                    QString::number(course.departmentId())
                )
            );
        }

        coursesLayout->addWidget(lecturerCoursesTable);
        lecturerTabs->addTab(coursesPage, "My Courses");

        // Assignments tab
        auto* assignmentsPage = new QWidget;
        auto* assignmentsLayout = new QVBoxLayout(assignmentsPage);

        auto* formHeading = new QLabel("Create an assignment");
        assignmentsLayout->addWidget(formHeading);

        auto* courseSelector = new QComboBox(assignmentsPage);

        for (const auto& course : lecturerCourses) {
            courseSelector->addItem(
                QString("%1 - %2")
                    .arg(QString::fromStdString(course.code()))
                    .arg(QString::fromStdString(course.name())),
                course.id()
            );
        }

        auto* titleInput = new QLineEdit(assignmentsPage);
        titleInput->setPlaceholderText("Assignment title");

        auto* descriptionInput = new QTextEdit(assignmentsPage);
        descriptionInput->setPlaceholderText("Assignment instructions");
        descriptionInput->setMaximumHeight(90);

        auto* deadlineInput = new QDateTimeEdit(
            QDateTime::currentDateTime(), assignmentsPage
        );
        deadlineInput->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
        deadlineInput->setCalendarPopup(true);

        auto* maxMarkInput = new QDoubleSpinBox(assignmentsPage);
        maxMarkInput->setRange(0.01, 1000000.0);
        maxMarkInput->setDecimals(2);
        maxMarkInput->setValue(100.0);

        auto* form = new QFormLayout;
        form->addRow("Course:", courseSelector);
        form->addRow("Title:", titleInput);
        form->addRow("Instructions:", descriptionInput);
        form->addRow("Deadline:", deadlineInput);
        form->addRow("Maximum mark:", maxMarkInput);

        assignmentsLayout->addLayout(form);

        auto* createButton = new QPushButton(
            "Create Assignment", assignmentsPage
        );
        assignmentsLayout->addWidget(createButton);

        auto* assignmentsHeading = new QLabel("Assignments for my courses");
        assignmentsLayout->addWidget(assignmentsHeading);

        auto* lecturerAssignmentsTable = createTable(
            {"ID", "Course ID", "Title", "Deadline", "Maximum Mark", "Released"},
            assignmentsPage
        );
        assignmentsLayout->addWidget(lecturerAssignmentsTable);

        auto refreshAssignments = [this, lecturerAssignmentsTable,
                                   lecturerCourses]() {
            lecturerAssignmentsTable->setRowCount(0);

            for (const auto& course : lecturerCourses) {
                const auto assignments =
                    assignments_.findByCourse(course.id());

                for (const auto& assignment : assignments) {
                    const int row = lecturerAssignmentsTable->rowCount();
                    lecturerAssignmentsTable->insertRow(row);

                    lecturerAssignmentsTable->setItem(
                        row, 0,
                        new QTableWidgetItem(QString::number(assignment.id()))
                    );
                    lecturerAssignmentsTable->setItem(
                        row, 1,
                        new QTableWidgetItem(
                            QString::number(assignment.courseId())
                        )
                    );
                    lecturerAssignmentsTable->setItem(
                        row, 2,
                        new QTableWidgetItem(
                            QString::fromStdString(assignment.title())
                        )
                    );
                    lecturerAssignmentsTable->setItem(
                        row, 3,
                        new QTableWidgetItem(
                            QString::fromStdString(assignment.deadline())
                        )
                    );
                    lecturerAssignmentsTable->setItem(
                        row, 4,
                        new QTableWidgetItem(
                            QString::number(assignment.maxMark())
                        )
                    );
                    lecturerAssignmentsTable->setItem(
                        row, 5,
                        new QTableWidgetItem(
                            assignment.released() ? "Yes" : "No"
                        )
                    );
                }
            }
        };

        connect(createButton, &QPushButton::clicked, this,
                [this, courseSelector, titleInput, descriptionInput,
                 deadlineInput, maxMarkInput, refreshAssignments]() {
            if (courseSelector->currentIndex() < 0) {
                QMessageBox::warning(
                    this, "No course assigned",
                    "You must have an assigned course before creating an assignment."
                );
                return;
            }

            const QString title = titleInput->text().trimmed();
            const QString description = descriptionInput->toPlainText().trimmed();

            if (title.isEmpty() || description.isEmpty()) {
                QMessageBox::warning(
                    this, "Missing information",
                    "Enter both an assignment title and instructions."
                );
                return;
            }

            try {
                const int courseId = courseSelector->currentData().toInt();

                const int assignmentId = assignments_.createForLecturer(
                    user_,
                    courseId,
                    title.toStdString(),
                    description.toStdString(),
                    deadlineInput->dateTime()
                        .toString("yyyy-MM-dd HH:mm:ss").toStdString(),
                    maxMarkInput->value()
                );

                QMessageBox::information(
                    this, "Assignment created",
                    QString("Assignment created successfully. ID: %1")
                        .arg(assignmentId)
                );

                titleInput->clear();
                descriptionInput->clear();
                refreshAssignments();

            } catch (const std::exception& error) {
                QMessageBox::warning(
                    this, "Could not create assignment",
                    QString::fromUtf8(error.what())
                );
            }
        });

        refreshAssignments();

        lecturerTabs->addTab(assignmentsPage, "Assignments");

        // Submissions & Marking tab
        auto* submissionsPage = new QWidget;
        auto* submissionsLayout = new QVBoxLayout(submissionsPage);

        auto* submissionsHeading = new QLabel(
            "View and mark student submissions"
        );
        submissionsLayout->addWidget(submissionsHeading);

        auto* assignmentSelector = new QComboBox(submissionsPage);

        for (const auto& course : lecturerCourses) {
            const auto courseAssignments =
                assignments_.findByCourse(course.id());

            for (const auto& assignment : courseAssignments) {
                const int index = assignmentSelector->count();

                assignmentSelector->addItem(
                    QString("%1 - %2")
                        .arg(QString::fromStdString(course.code()))
                        .arg(QString::fromStdString(assignment.title())),
                    assignment.id()
                );

                assignmentSelector->setItemData(
                    index,
                    assignment.maxMark(),
                    Qt::UserRole + 1
                );
            }
        }

        auto* assignmentForm = new QFormLayout;
        assignmentForm->addRow(
            "Select assignment:",
            assignmentSelector
        );
        submissionsLayout->addLayout(assignmentForm);

        auto* submissionsTable = createTable(
            {"Submission ID", "Student ID", "Version",
             "Submitted At", "Status"},
            submissionsPage
        );

        submissionsLayout->addWidget(
            new QLabel("Student submissions:")
        );
        submissionsLayout->addWidget(submissionsTable);

        auto* contentHeading = new QLabel("Submission content");
        submissionsLayout->addWidget(contentHeading);

        auto* contentViewer = new QTextEdit(submissionsPage);
        contentViewer->setReadOnly(true);
        contentViewer->setPlaceholderText(
            "Select a submission to view its content."
        );
        contentViewer->setMaximumHeight(120);
        submissionsLayout->addWidget(contentViewer);

        auto* markingForm = new QFormLayout;

        auto* markInput = new QDoubleSpinBox(submissionsPage);
        markInput->setRange(0.0, 1000000.0);
        markInput->setDecimals(2);
        markInput->setValue(0.0);

        auto* feedbackInput = new QTextEdit(submissionsPage);
        feedbackInput->setPlaceholderText(
            "Enter feedback for the student."
        );
        feedbackInput->setMaximumHeight(90);

        markingForm->addRow("Mark:", markInput);
        markingForm->addRow("Feedback:", feedbackInput);

        submissionsLayout->addLayout(markingForm);

        auto* saveMarkButton = new QPushButton(
            "Save Mark",
            submissionsPage
        );
        saveMarkButton->setEnabled(false);
        submissionsLayout->addWidget(saveMarkButton);

        auto refreshSubmissions =
            [this, assignmentSelector, submissionsTable,
             contentViewer, markInput, feedbackInput,
             saveMarkButton]() {

            submissionsTable->setRowCount(0);
            contentViewer->clear();
            markInput->setValue(0.0);
            feedbackInput->clear();
            saveMarkButton->setEnabled(false);

            if (assignmentSelector->currentIndex() < 0) {
                return;
            }

            const int assignmentId =
                assignmentSelector->currentData().toInt();

            const auto submissions =
                submissions_.findByAssignment(assignmentId);

            for (const auto& submission : submissions) {
                const int row = submissionsTable->rowCount();
                submissionsTable->insertRow(row);

                auto* idItem = new QTableWidgetItem(
                    QString::number(submission.id())
                );

                idItem->setData(
                    Qt::UserRole,
                    QString::fromStdString(submission.content())
                );

                submissionsTable->setItem(row, 0, idItem);

                submissionsTable->setItem(
                    row, 1,
                    new QTableWidgetItem(
                        QString::number(submission.studentId())
                    )
                );

                submissionsTable->setItem(
                    row, 2,
                    new QTableWidgetItem(
                        QString::number(submission.version())
                    )
                );

                submissionsTable->setItem(
                    row, 3,
                    new QTableWidgetItem(
                        QString::fromStdString(submission.submittedAt())
                    )
                );

                submissionsTable->setItem(
                    row, 4,
                    new QTableWidgetItem(
                        QString::fromStdString(
                            submissionStatusToString(submission.status())
                        )
                    )
                );
            }

            if (submissionsTable->rowCount() > 0) {
                submissionsTable->selectRow(0);
            }
        };

        connect(
            assignmentSelector,
            &QComboBox::currentIndexChanged,
            this,
            [refreshSubmissions](int) {
                refreshSubmissions();
            }
        );

        connect(
            submissionsTable,
            &QTableWidget::currentCellChanged,
            this,
            [submissionsTable, contentViewer, markInput,
             feedbackInput, saveMarkButton,
             assignmentSelector](int currentRow, int, int, int) {

                contentViewer->clear();
                feedbackInput->clear();
                markInput->setValue(0.0);
                saveMarkButton->setEnabled(false);

                if (currentRow < 0) {
                    return;
                }

                auto* idItem = submissionsTable->item(
                    currentRow, 0
                );

                auto* statusItem = submissionsTable->item(
                    currentRow, 4
                );

                if (!idItem || !statusItem) {
                    return;
                }

                contentViewer->setPlainText(
                    idItem->data(Qt::UserRole).toString()
                );

                const QString status =
                    statusItem->text();

                const bool locked =
                    status == "MODERATED" ||
                    status == "RELEASED";

                const double maximumMark =
                    assignmentSelector->currentData(
                        Qt::UserRole + 1
                    ).toDouble();

                markInput->setMaximum(maximumMark);

                markInput->setEnabled(!locked);
                feedbackInput->setEnabled(!locked);
                saveMarkButton->setEnabled(!locked);
            }
        );

        connect(
            saveMarkButton,
            &QPushButton::clicked,
            this,
            [this, submissionsTable, markInput,
             feedbackInput, refreshSubmissions]() {

                const int row =
                    submissionsTable->currentRow();

                if (row < 0) {
                    QMessageBox::warning(
                        this,
                        "No submission selected",
                        "Select a submission before saving a mark."
                    );
                    return;
                }

                const auto* idItem =
                    submissionsTable->item(row, 0);

                if (!idItem) {
                    return;
                }

                const int submissionId =
                    idItem->text().toInt();

                const QString feedback =
                    feedbackInput->toPlainText().trimmed();

                if (feedback.isEmpty()) {
                    QMessageBox::warning(
                        this,
                        "Feedback required",
                        "Enter feedback before saving the mark."
                    );
                    return;
                }

                try {
                    const QString timestamp =
                        QDateTime::currentDateTime().toString(
                            "yyyy-MM-dd HH:mm:ss"
                        );

                    assessment_.markSubmission(
                        user_,
                        submissionId,
                        markInput->value(),
                        feedback.toStdString(),
                        timestamp.toStdString()
                    );

                    QMessageBox::information(
                        this,
                        "Mark saved",
                        "The submission has been marked successfully."
                    );

                    refreshSubmissions();

                } catch (const std::exception& error) {
                    QMessageBox::warning(
                        this,
                        "Could not save mark",
                        QString::fromUtf8(error.what())
                    );
                }
            }
        );

        refreshSubmissions();

        lecturerTabs->addTab(
            submissionsPage,
            "Submissions & Marking"
        );

        root->addWidget(lecturerTabs);
    }

    void buildAdministratorDashboard(QVBoxLayout* root)
    {
        tabs_ = new QTabWidget(this);

        buildOverviewTab();
        buildUsersTab();
        buildDepartmentsTab();
        buildCoursesTab();
        buildRelationshipsTab();

        root->addWidget(tabs_);

        refreshAll();
    }

    void buildOverviewTab()
    {
        auto* page = new QWidget;
        auto* layout = new QVBoxLayout(page);

        auto* heading = new QLabel("Administrator Overview");
        heading->setStyleSheet(
            "font-size: 22px; font-weight: bold;"
        );

        userCount_ = new QLabel;
        departmentCount_ = new QLabel;
        courseCount_ = new QLabel;

        for (QLabel* label : {
                 userCount_, departmentCount_, courseCount_
             }) {
            label->setStyleSheet(
                "font-size: 18px; padding: 15px;"
                "background: #303846; border-radius: 6px;"
            );
        }

        auto* refresh = new QPushButton("Refresh overview");
        connect(refresh, &QPushButton::clicked, this, [this]() {
            refreshOverview();
        });

        layout->addWidget(heading);
        layout->addSpacing(15);
        layout->addWidget(userCount_);
        layout->addWidget(departmentCount_);
        layout->addWidget(courseCount_);
        layout->addWidget(refresh);
        layout->addStretch();

        tabs_->addTab(page, "Overview");
    }

    void buildUsersTab()
    {
        auto* page = new QWidget;
        auto* layout = new QVBoxLayout(page);

        auto* heading = new QLabel("User Management");
        heading->setStyleSheet(
            "font-size: 20px; font-weight: bold;"
        );

        auto* form = new QFormLayout;

        usernameInput_ = new QLineEdit;
        usernameInput_->setPlaceholderText("e.g. lecturer01");

        fullNameInput_ = new QLineEdit;
        fullNameInput_->setPlaceholderText("Enter full name");

        passwordInput_ = new QLineEdit;
        passwordInput_->setEchoMode(QLineEdit::Password);
        passwordInput_->setPlaceholderText(
            "At least 12 characters"
        );

        roleInput_ = new QComboBox;
        roleInput_->addItem("Administrator",
                            static_cast<int>(Role::Administrator));
        roleInput_->addItem("Lecturer",
                            static_cast<int>(Role::Lecturer));
        roleInput_->addItem("Head of Department",
                            static_cast<int>(Role::HeadOfDepartment));
        roleInput_->addItem("Student",
                            static_cast<int>(Role::Student));

        form->addRow("Username:", usernameInput_);
        form->addRow("Full name:", fullNameInput_);
        form->addRow("Password:", passwordInput_);
        form->addRow("Role:", roleInput_);

        auto* create = new QPushButton("Create user");
        auto* refresh = new QPushButton("Refresh users");

        auto* buttons = new QHBoxLayout;
        buttons->addWidget(create);
        buttons->addWidget(refresh);

        userTable_ = createTable(
            {"ID", "Username", "Full name", "Role"},
            page
        );

        connect(create, &QPushButton::clicked,
                this, [this]() { createUser(); });

        connect(refresh, &QPushButton::clicked,
                this, [this]() { refreshUsers(); });

        layout->addWidget(heading);
        layout->addLayout(form);
        layout->addLayout(buttons);
        layout->addWidget(userTable_);

        tabs_->addTab(page, "Users");
    }

    void buildDepartmentsTab()
    {
        auto* page = new QWidget;
        auto* layout = new QVBoxLayout(page);

        auto* heading = new QLabel("Department Management");
        heading->setStyleSheet(
            "font-size: 20px; font-weight: bold;"
        );

        departmentNameInput_ = new QLineEdit;
        departmentNameInput_->setPlaceholderText(
            "Enter department name"
        );

        auto* form = new QFormLayout;
        form->addRow("Department name:", departmentNameInput_);

        auto* create = new QPushButton("Create department");
        auto* refresh = new QPushButton("Refresh departments");

        auto* buttons = new QHBoxLayout;
        buttons->addWidget(create);
        buttons->addWidget(refresh);

        departmentTable_ = createTable(
            {"ID", "Department name"},
            page
        );

        connect(create, &QPushButton::clicked,
                this, [this]() { createDepartment(); });

        connect(refresh, &QPushButton::clicked,
                this, [this]() { refreshDepartments(); });

        layout->addWidget(heading);
        layout->addLayout(form);
        layout->addLayout(buttons);
        layout->addWidget(departmentTable_);

        tabs_->addTab(page, "Departments");
    }

    void buildCoursesTab()
    {
        auto* page = new QWidget;
        auto* layout = new QVBoxLayout(page);

        auto* heading = new QLabel("Course Management");
        heading->setStyleSheet(
            "font-size: 20px; font-weight: bold;"
        );

        courseCodeInput_ = new QLineEdit;
        courseCodeInput_->setPlaceholderText("e.g. CSC4000");

        courseNameInput_ = new QLineEdit;
        courseNameInput_->setPlaceholderText("Enter course name");

        courseDepartmentInput_ = new QComboBox;

        auto* form = new QFormLayout;
        form->addRow("Course code:", courseCodeInput_);
        form->addRow("Course name:", courseNameInput_);
        form->addRow("Department:", courseDepartmentInput_);

        auto* create = new QPushButton("Create course");
        auto* refresh = new QPushButton("Refresh courses");

        auto* buttons = new QHBoxLayout;
        buttons->addWidget(create);
        buttons->addWidget(refresh);

        courseTable_ = createTable(
            {"ID", "Code", "Course name", "Department ID"},
            page
        );

        connect(create, &QPushButton::clicked,
                this, [this]() { createCourse(); });

        connect(refresh, &QPushButton::clicked,
                this, [this]() { refreshCourses(); });

        layout->addWidget(heading);
        layout->addLayout(form);
        layout->addLayout(buttons);
        layout->addWidget(courseTable_);

        tabs_->addTab(page, "Courses");
    }

    void buildRelationshipsTab()
    {
        auto* page = new QWidget;
        auto* layout = new QVBoxLayout(page);

        auto* heading = new QLabel("Assignments and Enrolments");
        heading->setStyleSheet("font-size: 20px; font-weight: bold;");
        layout->addWidget(heading);

        auto* lecturerForm = new QFormLayout;
        lecturerInput_ = new QComboBox;
        lecturerCourseInput_ = new QComboBox;
        lecturerForm->addRow("Lecturer:", lecturerInput_);
        lecturerForm->addRow("Course:", lecturerCourseInput_);
        layout->addLayout(lecturerForm);

        auto* assignLecturer = new QPushButton("Assign lecturer to course");
        layout->addWidget(assignLecturer);

        connect(assignLecturer, &QPushButton::clicked, this, [this]() {
            if (lecturerInput_->currentIndex() < 0 ||
                lecturerCourseInput_->currentIndex() < 0) {
                QMessageBox::warning(this, "Missing selection",
                                     "Select a lecturer and a course.");
                return;
            }
            try {
                administration_.assignLecturerToCourse(
                    lecturerInput_->currentData().toInt(),
                    lecturerCourseInput_->currentData().toInt()
                );
                QMessageBox::information(
                    this, "Success", "Lecturer assigned to the course."
                );
            } catch (const std::exception& e) {
                QMessageBox::warning(
                    this, "Assignment failed", QString::fromUtf8(e.what())
                );
            }
        });

        auto* hodForm = new QFormLayout;
        hodInput_ = new QComboBox;
        hodDepartmentInput_ = new QComboBox;
        hodForm->addRow("Head of Department:", hodInput_);
        hodForm->addRow("Department:", hodDepartmentInput_);
        layout->addSpacing(10);
        layout->addLayout(hodForm);

        auto* assignHod = new QPushButton("Assign HOD to department");
        layout->addWidget(assignHod);

        connect(assignHod, &QPushButton::clicked, this, [this]() {
            if (hodInput_->currentIndex() < 0 ||
                hodDepartmentInput_->currentIndex() < 0) {
                QMessageBox::warning(this, "Missing selection",
                                     "Select an HOD and a department.");
                return;
            }
            try {
                administration_.assignHodToDepartment(
                    hodInput_->currentData().toInt(),
                    hodDepartmentInput_->currentData().toInt()
                );
                QMessageBox::information(
                    this, "Success", "HOD assigned to the department."
                );
            } catch (const std::exception& e) {
                QMessageBox::warning(
                    this, "Assignment failed", QString::fromUtf8(e.what())
                );
            }
        });

        auto* studentForm = new QFormLayout;
        studentInput_ = new QComboBox;
        studentCourseInput_ = new QComboBox;
        studentForm->addRow("Student:", studentInput_);
        studentForm->addRow("Course:", studentCourseInput_);
        layout->addSpacing(10);
        layout->addLayout(studentForm);

        auto* enrolStudent = new QPushButton("Enrol student in course");
        layout->addWidget(enrolStudent);

        connect(enrolStudent, &QPushButton::clicked, this, [this]() {
            if (studentInput_->currentIndex() < 0 ||
                studentCourseInput_->currentIndex() < 0) {
                QMessageBox::warning(this, "Missing selection",
                                     "Select a student and a course.");
                return;
            }
            try {
                administration_.enrolStudentInCourse(
                    studentInput_->currentData().toInt(),
                    studentCourseInput_->currentData().toInt()
                );
                QMessageBox::information(
                    this, "Success", "Student enrolled in the course."
                );
            } catch (const std::exception& e) {
                QMessageBox::warning(
                    this, "Enrolment failed", QString::fromUtf8(e.what())
                );
            }
        });

        auto* refresh = new QPushButton("Refresh available records");
        layout->addSpacing(10);
        layout->addWidget(refresh);
        connect(refresh, &QPushButton::clicked, this,
                [this]() { refreshRelationshipOptions(); });

        layout->addStretch();
        tabs_->addTab(page, "Assignments & Enrolments");
    }

    void refreshRelationshipOptions()
    {
        lecturerInput_->clear();
        hodInput_->clear();
        studentInput_->clear();

        for (const auto& user : users_.findAll()) {
            const QString label =
                QString("%1 (ID: %2)")
                    .arg(QString::fromStdString(user->fullName()))
                    .arg(user->id());

            if (user->role() == Role::Lecturer)
                lecturerInput_->addItem(label, user->id());
            else if (user->role() == Role::HeadOfDepartment)
                hodInput_->addItem(label, user->id());
            else if (user->role() == Role::Student)
                studentInput_->addItem(label, user->id());
        }

        lecturerCourseInput_->clear();
        studentCourseInput_->clear();

        for (const auto& course : courses_.findAll()) {
            const QString label =
                QString("%1 - %2 (ID: %3)")
                    .arg(QString::fromStdString(course.code()))
                    .arg(QString::fromStdString(course.name()))
                    .arg(course.id());

            lecturerCourseInput_->addItem(label, course.id());
            studentCourseInput_->addItem(label, course.id());
        }

        hodDepartmentInput_->clear();

        for (const auto& department : departments_.findAll()) {
            const QString label =
                QString("%1 (ID: %2)")
                    .arg(QString::fromStdString(department.name()))
                    .arg(department.id());

            hodDepartmentInput_->addItem(label, department.id());
        }
    }

    void refreshAll()
    {
        refreshOverview();
        refreshUsers();
        refreshDepartments();
        refreshCourses();
        refreshRelationshipOptions();
    }

    void refreshOverview()
    {
        userCount_->setText(
            QString("Registered users: %1")
                .arg(users_.countUsers())
        );

        departmentCount_->setText(
            QString("Departments: %1")
                .arg(departments_.findAll().size())
        );

        courseCount_->setText(
            QString("Courses: %1")
                .arg(courses_.findAll().size())
        );
    }

    void refreshUsers()
    {
        const auto users = users_.findAll();

        userTable_->setRowCount(
            static_cast<int>(users.size())
        );

        for (int row = 0; row < static_cast<int>(users.size()); ++row) {
            const auto& user = users[static_cast<std::size_t>(row)];

            userTable_->setItem(
                row, 0,
                new QTableWidgetItem(QString::number(user->id()))
            );

            userTable_->setItem(
                row, 1,
                new QTableWidgetItem(
                    QString::fromStdString(user->username())
                )
            );

            userTable_->setItem(
                row, 2,
                new QTableWidgetItem(
                    QString::fromStdString(user->fullName())
                )
            );

            userTable_->setItem(
                row, 3,
                new QTableWidgetItem(
                    QString::fromStdString(roleToString(user->role()))
                )
            );
        }
    }

    void refreshDepartments()
    {
        const auto departments = departments_.findAll();

        departmentTable_->setRowCount(
            static_cast<int>(departments.size())
        );

        courseDepartmentInput_->clear();

        for (int row = 0;
             row < static_cast<int>(departments.size());
             ++row) {
            const auto& department =
                departments[static_cast<std::size_t>(row)];

            departmentTable_->setItem(
                row, 0,
                new QTableWidgetItem(
                    QString::number(department.id())
                )
            );

            departmentTable_->setItem(
                row, 1,
                new QTableWidgetItem(
                    QString::fromStdString(department.name())
                )
            );

            courseDepartmentInput_->addItem(
                QString("%1 - %2")
                    .arg(department.id())
                    .arg(QString::fromStdString(department.name())),
                department.id()
            );
        }
    }

    void refreshCourses()
    {
        const auto courses = courses_.findAll();

        courseTable_->setRowCount(
            static_cast<int>(courses.size())
        );

        for (int row = 0; row < static_cast<int>(courses.size()); ++row) {
            const auto& course = courses[static_cast<std::size_t>(row)];

            courseTable_->setItem(
                row, 0,
                new QTableWidgetItem(
                    QString::number(course.id())
                )
            );

            courseTable_->setItem(
                row, 1,
                new QTableWidgetItem(
                    QString::fromStdString(course.code())
                )
            );

            courseTable_->setItem(
                row, 2,
                new QTableWidgetItem(
                    QString::fromStdString(course.name())
                )
            );

            courseTable_->setItem(
                row, 3,
                new QTableWidgetItem(
                    QString::number(course.departmentId())
                )
            );
        }
    }

    void createUser()
    {
        try {
            const QString username = usernameInput_->text().trimmed();
            const QString fullName = fullNameInput_->text().trimmed();
            const QString password = passwordInput_->text();

            const auto role = static_cast<Role>(
                roleInput_->currentData().toInt()
            );

            const int id = users_.createUser(
                username.toStdString(),
                password.toStdString(),
                fullName.toStdString(),
                role
            );

            QMessageBox::information(
                this,
                "User created",
                QString("User account created successfully. ID: %1")
                    .arg(id)
            );

            usernameInput_->clear();
            fullNameInput_->clear();
            passwordInput_->clear();

            refreshUsers();
            refreshOverview();

        } catch (const std::exception& error) {
            QMessageBox::warning(
                this,
                "Could not create user",
                QString::fromUtf8(error.what())
            );
        }
    }

    void createDepartment()
    {
        try {
            const QString name =
                departmentNameInput_->text().trimmed();

            if (name.isEmpty()) {
                QMessageBox::warning(
                    this,
                    "Missing information",
                    "Enter a department name."
                );
                return;
            }

            const int id = departments_.create(name.toStdString());

            QMessageBox::information(
                this,
                "Department created",
                QString("Department created successfully. ID: %1")
                    .arg(id)
            );

            departmentNameInput_->clear();

            refreshDepartments();
            refreshCourses();
            refreshOverview();

        } catch (const std::exception& error) {
            QMessageBox::warning(
                this,
                "Could not create department",
                QString::fromUtf8(error.what())
            );
        }
    }

    void createCourse()
    {
        try {
            const QString code = courseCodeInput_->text().trimmed();
            const QString name = courseNameInput_->text().trimmed();

            if (code.isEmpty() || name.isEmpty()) {
                QMessageBox::warning(
                    this,
                    "Missing information",
                    "Enter both a course code and course name."
                );
                return;
            }

            if (courseDepartmentInput_->currentIndex() < 0) {
                QMessageBox::warning(
                    this,
                    "Department required",
                    "Create a department before creating a course."
                );
                return;
            }

            const int departmentId =
                courseDepartmentInput_->currentData().toInt();

            const int id = courses_.create(
                code.toStdString(),
                name.toStdString(),
                departmentId
            );

            QMessageBox::information(
                this,
                "Course created",
                QString("Course created successfully. ID: %1")
                    .arg(id)
            );

            courseCodeInput_->clear();
            courseNameInput_->clear();

            refreshCourses();
            refreshOverview();

        } catch (const std::exception& error) {
            QMessageBox::warning(
                this,
                "Could not create course",
                QString::fromUtf8(error.what())
            );
        }
    }
};

class LoginWindow : public QDialog {
public:
    explicit LoginWindow(AuthenticationService& authentication)
        : authentication_(authentication)
    {
        setWindowTitle("Moderated LMS - Login");
        setFixedSize(430, 330);

        auto* layout = new QVBoxLayout(this);

        auto* heading = new QLabel("MODERATED LMS");
        heading->setAlignment(Qt::AlignCenter);
        heading->setStyleSheet(
            "font-size: 25px; font-weight: bold;"
            "color: #17365D; padding: 15px;"
        );

        auto* subtitle = new QLabel("Sign in to your account");
        subtitle->setAlignment(Qt::AlignCenter);

        username_ = new QLineEdit;
        username_->setPlaceholderText("Enter username");

        password_ = new QLineEdit;
        password_->setPlaceholderText("Enter password");
        password_->setEchoMode(QLineEdit::Password);

        auto* form = new QFormLayout;
        form->addRow("Username:", username_);
        form->addRow("Password:", password_);

        auto* loginButton = new QPushButton("Sign in");
        loginButton->setMinimumHeight(38);

        layout->addWidget(heading);
        layout->addWidget(subtitle);
        layout->addSpacing(15);
        layout->addLayout(form);
        layout->addSpacing(15);
        layout->addWidget(loginButton);

        connect(loginButton, &QPushButton::clicked,
                this, [this]() { attemptLogin(); });

        connect(password_, &QLineEdit::returnPressed,
                this, [this]() { attemptLogin(); });
    }

private:
    AuthenticationService& authentication_;
    QLineEdit* username_{};
    QLineEdit* password_{};

    void attemptLogin()
    {
        const QString username = username_->text().trimmed();
        const QString password = password_->text();

        if (username.isEmpty() || password.isEmpty()) {
            QMessageBox::warning(
                this,
                "Missing information",
                "Enter both your username and password."
            );
            return;
        }

        try {
            auto user = authentication_.login(
                username.toStdString(),
                password.toStdString()
            );

            password_->clear();

            if (!user) {
                QMessageBox::warning(
                    this,
                    "Login failed",
                    "Invalid username or password."
                );
                return;
            }

            DashboardWindow dashboard(
                *user,
                *users_,
                *departments_,
                *courses_,
                *enrolments_,
                *administration_,
                *assignments_,
                *submissions_,
                *assessment_
            );

            hide();
            dashboard.exec();
            show();
            username_->setFocus();

        } catch (const std::exception& error) {
            QMessageBox::critical(
                this,
                "Application error",
                QString::fromUtf8(error.what())
            );
        }
    }

public:
    void setRepositories(
        UserRepository& users,
        DepartmentRepository& departments,
        CourseRepository& courses,
        EnrolmentRepository& enrolments,
        AdministrationRepository& administration,
        AssignmentRepository& assignments,
        SubmissionRepository& submissions,
        AssessmentWorkflowService& assessment
    )
    {
        users_ = &users;
        departments_ = &departments;
        courses_ = &courses;
        enrolments_ = &enrolments;
        administration_ = &administration;
        assignments_ = &assignments;
        submissions_ = &submissions;
        assessment_ = &assessment;
    }

private:
    UserRepository* users_{};
    DepartmentRepository* departments_{};
    CourseRepository* courses_{};
    EnrolmentRepository* enrolments_{};
    AdministrationRepository* administration_{};
    AssignmentRepository* assignments_{};
    SubmissionRepository* submissions_{};
    AssessmentWorkflowService* assessment_{};
};

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);

    application.setApplicationName("Moderated LMS");
    application.setOrganizationName("Moderated LMS");

    try {
        const std::string path = moderatedLmsDatabasePath();

        Database database(path);
        database.initialise();

        UserRepository users(database);
        DepartmentRepository departments(database);
        CourseRepository courses(database);
        EnrolmentRepository enrolments(database);
        AdministrationRepository administration(database);
        AssignmentRepository assignments(database);
        SubmissionRepository submissions(database);
        AssessmentWorkflowService assessment(database);

        AuthenticationService authentication(users);

        if (users.countUsers() == 0) {
            QMessageBox::information(
                nullptr,
                "Initial setup required",
                "No user accounts exist yet.\n\n"
                "Run the CLI application (lms) to create "
                "the first administrator account."
            );
            return 0;
        }

        LoginWindow login(authentication);
        login.setRepositories(
            users, departments, courses, enrolments, administration,
            assignments, submissions, assessment
        );
        login.show();

        return application.exec();

    } catch (const std::exception& error) {
        QMessageBox::critical(
            nullptr,
            "Startup error",
            QString::fromUtf8(error.what())
        );
        return 1;
    }
}
