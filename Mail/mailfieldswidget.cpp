#include "mailfieldswidget.hpp"

#include "AddressBook/AddressBookModel.hpp"

#include "ui_mailfieldswidget.h"

#include <bts/application.hpp>
#include <bts/profile.hpp>
#include <bts/address.hpp>

/// QT headers:
#include <QAction>
#include <QMenu>

MailFieldsWidget::MailFieldsWidget(QWidget& parent, QAction& actionSend, AddressBookModel& abModel) :
  QWidget(&parent),
  ui(new Ui::MailFieldsWidget),
  ActionSend(actionSend)
  {
  ui->setupUi(this);

  fillSenderIdentities();
  validateSendButtonState();

  ContactListEdit* recipientEdits[3];
  recipientEdits[0] = ui->toEdit;
  recipientEdits[1] = ui->ccEdit;
  recipientEdits[2] = ui->bccEdit;

  QCompleter* completer = abModel.getContactCompleter();

  for(unsigned int i = 0; i < sizeof(recipientEdits)/sizeof(ContactListEdit*); ++i)
    {
    ContactListEdit* edit = recipientEdits[i];
    edit->setCompleter(completer);
    connect(edit->document(), SIGNAL(contentsChanged()), this, SLOT(onRecipientListChanged()));
    }
  }

MailFieldsWidget::~MailFieldsWidget()
  {
  delete ui;
  }

void MailFieldsWidget::showFromControls(bool show)
  {
  showChildLayout(ui->fromLayout, show, 0);
  }

void MailFieldsWidget::showCcControls(bool show)
  {
  showChildLayout(ui->ccLayout, show, 2);
  }

void MailFieldsWidget::showBccControls(bool show)
  {
  showChildLayout(ui->bccLayout, show, 3);
  }

QString MailFieldsWidget::getSubject() const
  {
  return ui->subjectEdit->text();
  }

void MailFieldsWidget::showChildLayout(QLayout* layout, bool show, int preferredPosition)
  {
  ui->mailFieldsLayout->setEnabled(false);
  if(show)
    {
    int maxCnt = ui->mailFieldsLayout->count();
    if(preferredPosition >= maxCnt)
      preferredPosition = maxCnt;

    ui->mailFieldsLayout->insertLayout(preferredPosition, layout);
    }
  else
    {
    int itemIdx = 0;
    for(itemIdx = 0; itemIdx < ui->mailFieldsLayout->count(); ++itemIdx)
      {
      if(ui->mailFieldsLayout->itemAt(itemIdx) == layout)
        break;
      }

    ui->mailFieldsLayout->takeAt(itemIdx);
    }

  showLayoutWidgets(layout, show);

  ui->mailFieldsLayout->setEnabled(true);
  ui->mailFieldsLayout->invalidate();
  }

void MailFieldsWidget::showLayoutWidgets(QLayout* layout, bool show)
  {
  for(int i = 0; i < layout->count(); ++i)
    {
    QLayoutItem* li = layout->itemAt(i);
    QWidget* w = li->widget();
    w->setVisible(show);
    w->setEnabled(show);
    }
  }

void MailFieldsWidget::validateSendButtonState()
  {
  bool anyRecipient = ui->toEdit->document()->isEmpty() == false;
  anyRecipient = anyRecipient || ui->ccEdit->document()->isEmpty() == false;
  anyRecipient = anyRecipient || ui->bccEdit->document()->isEmpty() == false;

  /// Make the send button enabled only if there is any recipient
  ui->sendButton->setEnabled(anyRecipient);
  }

void MailFieldsWidget::fillSenderIdentities()
  {
  QMenu* menu = new QMenu(this);
  ui->fromButton->setMenu(menu);

  QAction* first = nullptr;

  std::vector<bts::addressbook::wallet_identity> identities = bts::application::instance()->get_profile()->identities();
  for(const auto& identity : identities)
    {
    std::string identity_label = identity.first_name + " " + identity.last_name;
    std::string entry(identity_label);

    if(identity_label.empty() == false)
      entry += '(';

    entry += identity.dac_id_string;

    if(identity_label.empty() == false)
      entry += ')';

    QAction* action = menu->addAction(tr(entry.c_str()));
    action->setCheckable(true);
    Action2Identity.insert(TAction2IdentityIndex::value_type(action, identity));
    
    if(first == nullptr)
      first = action;
    }

  connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(onFromBtnTriggered(QAction*)));

  onFromBtnTriggered(first);
  menu->setActiveAction(first);
  }

void MailFieldsWidget::on_sendButton_clicked()
  {
  ActionSend.trigger();
  }

void MailFieldsWidget::onRecipientListChanged()
  {
  validateSendButtonState();
  }

void MailFieldsWidget::onSubjectChanged(const QString &subject)
  {
  emit subjectChanged(subject);
  }

void MailFieldsWidget::onFromBtnTriggered(QAction* action)
  {
  /// Clear checked state for all identities
  for(const auto& v : Action2Identity)
    v.first->setChecked(false);

  action->setChecked(true);

  TAction2IdentityIndex::const_iterator foundPos = Action2Identity.find(action);
  assert(foundPos != Action2Identity.end());

  SenderIdentity = foundPos->second;

  ui->fromEdit->setText(action->text());
  }

