#include "approvalwork.h"

ApprovalWork::ApprovalWork(QObject *parent)
	: QObject(parent)
{
	db_service.addDatabase(DB, "ApprovalWork_DB");
	db_service.addDatabase(DB_SECOND, "ApprovalWork_DB_SECOND");
}

void ApprovalWork::getManagePageApplyItems(const QString& uid)
{
	DB.open();
	QSqlQuery query(DB);
	query.exec(QString("SELECT * FROM magic_applyItems WHERE publisher=1 OR publisher=%1").arg(uid));
	QByteArray array;
	applyItems.clear();
	while (query.next()) {
		QByteArray array;
		QSqlRecord record = query.record();
		QDataStream stream(&array, QIODevice::WriteOnly);
		stream << record.value("item_id").toString() << record.value("title").toString() << record.value("options").toString() << record.value("publisher").toString() << record.value("auditor_list").toString() << record.value("isHide").toString();
		applyItems.push_back(array);
		
		//��ȡ���������
		QStringList auditor_list = record.value("auditor_list").toString().split(";", QString::SkipEmptyParts);
		for (auto uid : auditor_list)
		{
			QSqlQuery query(DB);
			query.exec(QString("SELECT name FROM magic_users WHERE uid=%1").arg(uid));
			if (query.next())
				auditorName[uid] = query.value("name").toString();
			query.clear();
		}
	}
	query.clear();
	DB.close();
	
	getManagePageAuditorList();
	emit getManagePageApplyItemsFinished();
}

void ApprovalWork::getAllApplyFormList(const QString& uid)
{
	DB.open();
	QSqlQuery query(DB);

	applyFormList.clear();
	applyFormListDone.clear();

	query.exec("SELECT * FROM magic_applyItems");	//��ȡ����������Ŀ������
	while (query.next())
	{
		QByteArray array;
		QDataStream stream(&array, QIODevice::WriteOnly);
		QSqlRecord record = query.record();
		stream << record.value("title").toString() << record.value("options").toString();
		applyItemTitle.insert(record.value("item_id").toString(), record.value("title").toString());
		simpleApplyItems.insert(record.value("item_id").toString(), array);
		applyAuditorList.insert(record.value("item_id").toString(), record.value("auditor_list").toString().split(";", QString::SkipEmptyParts));
	}
	QList<QByteArray> allApplyFormList;
	query.exec("SELECT * FROM magic_apply ORDER BY apply_id DESC");	//��ȡ���������
	while (query.next())
	{
		if (applyAuditorList[query.value("item_id").toString()].indexOf(uid) != -1)	//��Ҫ����˵������
		{
			QByteArray array;
			QDataStream stream(&array, QIODevice::WriteOnly);
			stream << query.value("apply_id").toString() << query.value("uid").toString() << query.value("item_id").toString() << query.value("options").toString() << query.value("operate_time").toDateTime().toString("yyyy-MM-dd hh:mm:ss");
			allApplyFormList.push_back(array);
		}
	}
	for (auto applyForm : allApplyFormList)
	{
		QDataStream stream(&applyForm, QIODevice::ReadOnly);
		QString apply_id, m_uid, item_id, options, operate_time;
		stream >> apply_id >> m_uid >> item_id >> options >> operate_time;
		QStringList auditor_list = applyAuditorList[item_id];
		for (auto auditor: auditor_list)	//�����������̣��Ƿ�Ӧ�õ�ǰ�û����
		{
			query.exec(QString("SELECT * FROM magic_applyProcess WHERE apply_id = '%1' AND auditor = '%2'").arg(apply_id, auditor));
			bool isNext = query.next();
			if (isNext && query.value("result").toString() == "1")
			{
				//����˼�¼����ͨ��
				if (auditor != uid)
					continue;
				else 
				{
					applyFormListDone.push_back(applyForm);	//�Ѿ��������
					break;
				}
			}
			else
			{
				//����˼�¼�����Ѿ��ܾ�
				QString res;
				if (isNext)
					res = query.value("result").toString();
				if (auditor == uid && res.isEmpty())
					applyFormList.push_back(applyForm);	//��Ҫ�����
				else
				{
					if(auditor == uid && !res.isEmpty())
						applyFormListDone.push_back(applyForm);	//�Ѿ��������
					else
						break;	//��û������ˣ��������Ѿ���ֹ
				}
			}
		}
		
	}
	
	query.clear();
	DB.close();

	emit getApplyFormListFinished();
}

void ApprovalWork::getUserPageApplyItems(const QString& uid)
{
	DB.open();
	QSqlQuery query(DB);
	query.exec(QString("SELECT * FROM magic_applyItems WHERE isHide=0"));
	applyItems.clear();
	while (query.next()) {
		QByteArray array;
		QSqlRecord record = query.record();
		QDataStream stream(&array, QIODevice::WriteOnly);
		stream << record.value("item_id").toString() << record.value("title").toString() << record.value("options").toString() << record.value("publisher").toString() << record.value("auditor_list").toString() << record.value("isHide").toString();
		applyItems.push_back(array);
		applyItemTitle.insert(record.value("item_id").toString(), record.value("title").toString());
		applyAuditorList.insert(record.value("item_id").toString(), record.value("auditor_list").toString().split(";", QString::SkipEmptyParts));
		//��ȡ���������
		QStringList auditor_list = record.value("auditor_list").toString().split(";", QString::SkipEmptyParts);
		for (auto uid : auditor_list)
		{
			QSqlQuery query(DB);
			query.exec(QString("SELECT name FROM magic_users WHERE uid=%1").arg(uid));
			if (query.next())
				auditorName[uid] = query.value("name").toString();
			query.clear();
		}
	}
	query.clear();
	//��ȡ�û����ύ������
	applyForms.clear();
	query.exec(QString("SELECT * FROM magic_apply WHERE uid=%1 ORDER BY apply_id DESC").arg(uid));	//id����
	while(query.next())
	{
		QByteArray array;
		QSqlRecord record = query.record();
		QDataStream stream(&array, QIODevice::WriteOnly);
		stream << record.value("apply_id").toString() << record.value("uid").toString() << record.value("item_id").toString() << record.value("options").toString() << record.value("status").toString() << record.value("token").toString() << record.value("operate_time").toDateTime().toString("yyyy-MM-dd hh:mm:ss");
		applyForms.push_back(array);
		currentFormOptionsText.insert(record.value("apply_id").toString(), record.value("options").toString());
		getApplyProcess(record.value("apply_id").toString(), record.value("item_id").toString());	//��ȡ��ǰ�������������
	}
	query.clear();
	DB.close();
		
	emit getUserPageApplyItemsFinished();
}

void ApprovalWork::getApplyProcess(const QString& apply_id, const QString& item_id)
{
	DB_SECOND.open();
	QSqlQuery query(DB_SECOND);
	currentProcess.clear();
	int step = 0;	//�ѳɹ�ͨ��������
	for (auto uid : applyAuditorList[item_id])
	{
		QByteArray processOneStep;	//����������е�һ��
		query.exec(QString("SELECT * FROM magic_applyProcess WHERE apply_id = '%1' AND auditor = '%2'").arg(apply_id, uid));
		if(query.next())
		{
			QSqlRecord record = query.record();
			QDataStream stream(&processOneStep, QIODevice::WriteOnly);
			stream << record.value("result").toString() << record.value("result_text").toString() << record.value("operate_time").toDateTime().toString("yyyy-MM-dd hh:mm:ss");
			currentProcess.push_back(processOneStep);
			if (record.value("result").toString() == "0")	//0Ϊ�ܾ�ͨ����������ֹ
			{
				query.exec(QString("UPDATE magic_apply SET status=2 WHERE apply_id='%1'").arg(apply_id));	//�������״̬��Ϊ����ֹ
				break;	//������ֹ
			}
			step++;
		}
		else
			break;	//������ֹ
	}
	if(step == applyAuditorList[item_id].count())
		query.exec(QString("UPDATE magic_apply SET status=1 WHERE apply_id='%1'").arg(apply_id));	//��ͨ��
	applyFormsProcess.insert(apply_id, currentProcess);	//����������˵Ĳ�������Ӧid��ֵ
	
	query.clear();
	DB_SECOND.close();
}

void ApprovalWork::getManagePageAuditorList()
{
	DB.open();
	QSqlQuery query(DB);
	QList<QString> groups;
	query.exec("SELECT * FROM magic_group WHERE apply_manage=1;");
	while (query.next())
	{
		QSqlRecord record = query.record();
		groups.push_back(record.value("group_id").toString());	//������Ȩ�޵��û���
	}
	query.exec("SELECT * FROM magic_users;");
	auditorList.clear();
	while (query.next())
	{
		if (groups.indexOf(query.value("user_group").toString()) != -1)
		{
			auditorList.push_back(query.value("uid").toString());
			auditorName[query.value("uid").toString()] = query.value("name").toString();
		}
	}
	query.clear();
	DB.close();
}

void ApprovalWork::addOrModifyApplyItem(int type, QByteArray array)
{
	QDataStream stream(&array, QIODevice::ReadOnly);
	QString title, options, auditorList, publisher, isHide;
	stream >> title >> options >> publisher >> auditorList >> isHide;
	DB.open();
	QSqlQuery query(DB);
	bool res = false;
	if (type == 0)
		res = query.exec(QString("INSERT INTO magic_applyItems (title, options, publisher, auditor_list, isHide) VALUES ('%1', '%2', '%3', '%4', '%5')").arg(title, options, publisher, auditorList, isHide));
	else
		res = query.exec(QString("UPDATE magic_applyItems SET options='%1', auditor_list='%2' WHERE item_id='%3'").arg(options, auditorList, modifyItemID));

	query.clear();
	DB.close();
	emit addOrModifyApplyItemFinished(res);
}

void ApprovalWork::getApplyToken(const QString& id)
{
	DB.open();
	QSqlQuery query(DB);
	QString token;
	bool res = false;
	res = query.exec(QString("SELECT token FROM magic_apply WHERE apply_id = '%1'").arg(id));

	if (query.next() && query.value("token").toString() != "")
		token = query.value("token").toString();
	else
	{
		QString dateTime = QString::number(service::getWebTime());
		token = QCryptographicHash::hash((id + "_" + dateTime).toUtf8(), QCryptographicHash::Md5).toHex();
		res = query.exec(QString("UPDATE magic_apply SET token = '%1' WHERE apply_id = '%2'").arg(token, id));
	}
	query.clear();
	DB.close();
	if (!res)
		token = "error";
	emit getApplyTokenFinished(token);
}

QList<QByteArray> ApprovalWork::getApplyItems()
{
	return applyItems;
}

QList<QByteArray> ApprovalWork::getApplyForms()
{
	return applyForms;
}

QByteArray ApprovalWork::getSimpleApplyItems(const QString& item_id)
{
	return simpleApplyItems[item_id];
}

QList<QByteArray> ApprovalWork::getApplyFormList()
{
	return applyFormList;
}

QList<QByteArray> ApprovalWork::getApplyFormListDone()
{
	return applyFormListDone;
}

QList<QString> ApprovalWork::getAuditorList()
{
	return auditorList;
}

QString ApprovalWork::getAuditorName(const QString& uid)
{
	return auditorName[uid];
}

QString ApprovalWork::getApplyItemTitle(const QString& id)
{
	return applyItemTitle[id];
}

void ApprovalWork::setModifyItemID(const QString& id)
{
	modifyItemID = id;
}

void ApprovalWork::deleteOrSwitchApplyItem(int type, const QString& id)
{
	DB.open();
	QSqlQuery query(DB);
	bool res = false;
	if(type == 0)
		res = query.exec("DELETE FROM magic_applyItems WHERE item_id=" + id);
	else
	{
		query.exec("SELECT isHide FROM magic_applyItems WHERE item_id=" + id);
		query.next();
		if (query.value("isHide").toString() == "0")
			res = query.exec("UPDATE magic_applyItems SET isHide=1 WHERE item_id=" + id);
		else
			res = query.exec("UPDATE magic_applyItems SET isHide=0 WHERE item_id=" + id);
	}
	query.clear();
	DB.close();

	emit deleteOrSwitchApplyItemFinished(res);
}

void ApprovalWork::agreeOrRejectApply(const QString& apply_id, const QString& auditor, const QString& result, const QString& text)
{
	DB.open();
	QSqlQuery query(DB);
	bool res = query.exec(QString("INSERT INTO magic_applyProcess (apply_id, auditor, result, result_text, operate_time) VALUES('%1', '%2', '%3', '%4', NOW())").arg(apply_id, auditor, result, text));

	emit agreeOrRejectApplyFinished(res);
}

void ApprovalWork::submitOrCancelApply(int type, const QString& apply_id, QByteArray array)
{
	DB.open();
	QSqlQuery query(DB);
	bool res = false;
	if (type == 1)
	{
		QString uid, item_id, options, operate_time;
		QDataStream stream(&array, QIODevice::ReadOnly);
		stream >> uid >> item_id >> options >> operate_time;
		res = query.exec(QString("INSERT INTO magic_apply (uid, item_id, options, status, operate_time) VALUES ('%1', '%2', '%3', '%4', '%5')").arg(uid, item_id, options, QString("0"), operate_time));
		qDebug() << query.lastError().text();
	}else
	{
		res = query.exec(QString("DELETE FROM magic_apply WHERE apply_id='%1'").arg(apply_id));
	}
	query.clear();
	DB.close();
	
	emit submitOrCancelApplyFinished(res);
}

void ApprovalWork::authApplyToken(const QString& token)
{
	DB_SECOND.open();
	QSqlQuery query(DB_SECOND);
	query.exec(QString("SELECT * FROM magic_apply WHERE token = '%1'").arg(token));
	bool res = query.next();
	if (res)
	{
		QString apply_id = query.value("apply_id").toString();
		QString uid = query.value("uid").toString();
		QString item_id = query.value("item_id").toString();
		QString status = query.value("status").toString();

		if(status == "0")
			status = "�����";
		else if (status == "1")
			status = "��ͨ��";
		else if (status == "2")
			status = "����ֹ��δͨ����";
		QString operate_time = query.value("operate_time").toDateTime().toString("yyyy-MM-dd hh:mm:ss");
		QString item = QString("[%1] %2").arg(item_id, applyItemTitle[item_id]);
		authApplyTokenResultList << apply_id << uid << item << status << operate_time;
		emit authApplyTokenFinished(res);
	}
	else
		emit authApplyTokenFinished(res);
	query.clear();
	DB_SECOND.close();
}

QList<QByteArray> ApprovalWork::getCurrentApplyProcess(const QString& id)
{
	return applyFormsProcess[id];
}

QString ApprovalWork::getCurrentFormOptionsText(const QString& id)
{
	return currentFormOptionsText[id];
}

QList<QString> ApprovalWork::getAuthApplyTokenResultList()
{
	return authApplyTokenResultList;
}

ApprovalWork::~ApprovalWork()
{}